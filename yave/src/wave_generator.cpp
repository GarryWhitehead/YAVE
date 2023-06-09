/* Copyright (c) 2022 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "wave_generator.h"

#include "compute.h"
#include "engine.h"
#include "mapped_texture.h"
#include "scene.h"
#include "yave/texture_sampler.h"

#include <image_utils/noise_generator.h>

#include <random>

namespace yave
{

IWaveGenerator::IWaveGenerator(IEngine& engine)
    : engine_(engine),
      initialSpecCompute_(std::make_unique<Compute>(engine)),
      specCompute_(std::make_unique<Compute>(engine)),
      butterflyCompute_(std::make_unique<Compute>(engine)),
      fftHorizCompute_(std::make_unique<Compute>(engine)),
      fftVertCompute_(std::make_unique<Compute>(engine)),
      displaceCompute_(std::make_unique<Compute>(engine)),
      genMapCompute_(std::make_unique<Compute>(engine)),
      log2N_(std::log(Resolution) / log(2)),
      pingpong_(0),
      updateSpectrum_(true)
{
    auto reverse = [this](int idx) -> uint32_t {
        uint32_t res = 0;
        for (int i = 0; i < log2N_; ++i)
        {
            res = (res << 1) + (idx & 1);
            idx >>= 1;
        }
        return res;
    };

    for (size_t index = 0; index < Resolution; ++index)
    {
        reversedBits_[index] = reverse(index);
    }

    // generate gaussian noise for initial spectrum (h0k)
    std::random_device rd {};
    std::mt19937 gen {rd()};
    std::normal_distribution<float> d {0, 1};

    for (int i = 0; i < Resolution; ++i)
    {
        for (int j = 0; j < Resolution * 4; j += 4)
        {
            int index = i * (Resolution * 4) + j;
            noiseMap_[index] = d(gen);
            noiseMap_[index + 1] = d(gen);
            noiseMap_[index + 2] = d(gen);
            noiseMap_[index + 3] = d(gen);
        }
    }
    noiseTexture_ = engine.createMappedTextureI();
    noiseTexture_->setTextureI(
        noiseMap_,
        Resolution * Resolution * 4 * sizeof(float),
        Resolution,
        Resolution,
        1,
        1,
        Texture::TextureFormat::RGBA32F,
        backend::ImageUsage::Storage);

    butterflyLut_ = engine.createMappedTextureI();
    butterflyLut_->setEmptyTexture(
        log2N_, Resolution, Texture::TextureFormat::RGBA32F, backend::ImageUsage::Storage, 1, 1);

    // output textures for h0k and h0-k
    h0kTexture_ = engine.createMappedTextureI();
    h0minuskTexture_ = engine.createMappedTextureI();
    h0kTexture_->setEmptyTexture(
        Resolution,
        Resolution,
        Texture::TextureFormat::RGBA32F,
        backend::ImageUsage::Storage,
        1,
        1);
    h0minuskTexture_->setEmptyTexture(
        Resolution,
        Resolution,
        Texture::TextureFormat::RGBA32F,
        backend::ImageUsage::Storage,
        1,
        1);

    // displacement
    fftOutputImage_ = engine_.createMappedTextureI();
    fftOutputImage_->setEmptyTexture(
        Resolution, Resolution, Texture::TextureFormat::RG32F, backend::ImageUsage::Storage, 1, 1);
    heightMap_ = engine_.createMappedTextureI();
    heightMap_->setEmptyTexture(
        Resolution, Resolution, Texture::TextureFormat::R32F, backend::ImageUsage::Storage, 1, 1);

    // map generation
    displacementMap_ = engine_.createMappedTextureI();
    displacementMap_->setEmptyTexture(
        Resolution,
        Resolution,
        Texture::TextureFormat::RGBA32F,
        backend::ImageUsage::Storage,
        1,
        1);
    gradientMap_ = engine_.createMappedTextureI();
    gradientMap_->setEmptyTexture(
        Resolution,
        Resolution,
        Texture::TextureFormat::RGBA32F,
        backend::ImageUsage::Storage,
        1,
        1);
}

IWaveGenerator::~IWaveGenerator() {}

void IWaveGenerator::render(
    rg::RenderGraph& rGraph, IScene& scene, float dt, util::Timer<NanoSeconds>& timer)
{
    float N = static_cast<float>(Resolution);
    float log2N = static_cast<float>(log2N_);

    // only generate the initial spectrum data if something has changed - i.e wind speed or
    // direction
    if (updateSpectrum_)
    {
        rGraph.addExecutorPass("initial_spectrum", [=](vkapi::VkDriver& driver) {
            auto& cmds = driver.getCommands();
            auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

            initialSpecCompute_->addStorageImage(
                driver,
                "NoiseImage",
                noiseTexture_->getBackendHandle(),
                0,
                ImageStorageSet::StorageType::ReadOnly);

            // the output textures - h0k and h0-k
            initialSpecCompute_->addStorageImage(
                driver,
                "H0kImage",
                h0kTexture_->getBackendHandle(),
                1,
                ImageStorageSet::StorageType::WriteOnly);
            initialSpecCompute_->addStorageImage(
                driver,
                "H0minuskImage",
                h0minuskTexture_->getBackendHandle(),
                2,
                ImageStorageSet::StorageType::WriteOnly);

            initialSpecCompute_->addUboParam(
                "N", backend::BufferElementType::Int, (void*)&Resolution);
            initialSpecCompute_->addUboParam(
                "windSpeed", backend::BufferElementType::Float, (void*)&options.windSpeed);
            initialSpecCompute_->addUboParam(
                "windDirection", backend::BufferElementType::Float2, (void*)&options.windDirection);
            initialSpecCompute_->addUboParam(
                "L", backend::BufferElementType::Int, (void*)&options.L);
            initialSpecCompute_->addUboParam(
                "A", backend::BufferElementType::Float, (void*)&options.A);

            auto* bundle = initialSpecCompute_->build(engine_, "initial_spectrum.comp");
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, bundle, Resolution / 16, Resolution / 16, 1);
        });

        // Note: the butterfly image only needs updating if user deinfed chnages in resolution are
        // allowed at some point. This may need moving under its own flag.
        rGraph.addExecutorPass("fft_butterfly", [=](vkapi::VkDriver& driver) {
            auto& cmds = driver.getCommands();
            auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

            butterflyCompute_->addStorageImage(
                driver,
                "ButterflyImage",
                butterflyLut_->getBackendHandle(),
                0,
                ImageStorageSet::StorageType::WriteOnly);

            butterflyCompute_->addSsbo(
                "bitReversed",
                backend::BufferElementType::Uint,
                StorageBuffer::AccessType::ReadWrite,
                0,
                "ssbo",
                reversedBits_,
                Resolution);

            butterflyCompute_->addUboParam("N", backend::BufferElementType::Float, (void*)&N);
            butterflyCompute_->addUboParam(
                "log2N", backend::BufferElementType::Float, (void*)&log2N);

            auto* bundle = butterflyCompute_->build(engine_, "fft_butterfly.comp");
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, bundle, log2N_, Resolution / 16, 1);
        });

        updateSpectrum_ = false;
    }

    rGraph.addExecutorPass("spectrum", [=](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        // input images from the initial spectrum compute call
        specCompute_->addStorageImage(
            driver,
            "H0kImage",
            h0kTexture_->getBackendHandle(),
            0,
            ImageStorageSet::StorageType::ReadOnly);
        specCompute_->addStorageImage(
            driver,
            "H0minuskImage",
            h0minuskTexture_->getBackendHandle(),
            1,
            ImageStorageSet::StorageType::ReadOnly);

        // output images - dxyz
        specCompute_->addSsbo(
            "out_dxyz",
            backend::BufferElementType::Float2,
            StorageBuffer::AccessType::ReadWrite,
            0,
            "ssbo",
            nullptr,
            dxyzBufferSize);

        specCompute_->addUboParam("N", backend::BufferElementType::Int, (void*)&Resolution);
        specCompute_->addUboParam("L", backend::BufferElementType::Int, (void*)&options.L);
        specCompute_->addUboParam("time", backend::BufferElementType::Float, (void*)&dt);
        specCompute_->addUboParam("offset_dx", backend::BufferElementType::Int, (void*)&dxOffset);
        specCompute_->addUboParam("offset_dy", backend::BufferElementType::Int, (void*)&dyOffset);
        specCompute_->addUboParam("offset_dz", backend::BufferElementType::Int, (void*)&dzOffset);

        auto* bundle = specCompute_->build(engine_, "fft_spectrum.comp");

        vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);
        driver.dispatchCompute(
            cmds.getCmdBuffer().cmdBuffer, bundle, Resolution / 16, Resolution / 16, 1);
    });

    rGraph.addExecutorPass("fft", [=](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        // setup horizontal fft
        fftHorizCompute_->addStorageImage(
            driver,
            "ButterflySampler",
            butterflyLut_->getBackendHandle(),
            0,
            ImageStorageSet::StorageType::ReadOnly);

        fftHorizCompute_->addSsbo(
            "pingpong0",
            backend::BufferElementType::Float2,
            StorageBuffer::AccessType::ReadWrite,
            0,
            "ssbo_a",
            nullptr,
            dxyzBufferSize);

        fftHorizCompute_->copySsbo(
            *specCompute_, 0, 1, StorageBuffer::AccessType::ReadWrite, "SsboBufferB", "ssbo_b");

        fftHorizCompute_->addUboParam("N", backend::BufferElementType::Float, (void*)&N);
        fftHorizCompute_->addPushConstantParam("stage", backend::BufferElementType::Int);
        fftHorizCompute_->addPushConstantParam("pingpong", backend::BufferElementType::Int);
        fftHorizCompute_->addPushConstantParam("offset", backend::BufferElementType::Uint);

        auto* horizBundle = fftHorizCompute_->build(engine_, "fft_horiz.comp");

        // setup vertical fft
        fftVertCompute_->addStorageImage(
            driver,
            "ButterflySampler",
            butterflyLut_->getBackendHandle(),
            0,
            ImageStorageSet::StorageType::ReadOnly);

        fftVertCompute_->copySsbo(
            *fftHorizCompute_, 0, 0, StorageBuffer::AccessType::ReadWrite, "SsboBufferA", "ssbo_a");

        fftVertCompute_->copySsbo(
            *fftHorizCompute_, 1, 1, StorageBuffer::AccessType::ReadWrite, "SsboBufferB", "ssbo_b");

        fftVertCompute_->addUboParam("N", backend::BufferElementType::Float, (void*)&N);
        fftVertCompute_->addPushConstantParam("stage", backend::BufferElementType::Int);
        fftVertCompute_->addPushConstantParam("pingpong", backend::BufferElementType::Int);
        fftVertCompute_->addPushConstantParam("offset", backend::BufferElementType::Uint);

        auto* vertBundle = fftVertCompute_->build(engine_, "fft_vert.comp");
        vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);

        // dispatch horizontal fft
        for (size_t n = 0; n < log2N_; ++n)
        {
            pingpong_ = pingpong_ == 0 ? 1 : 0;
            fftHorizCompute_->updatePushConstantParam("stage", &n);
            fftHorizCompute_->updatePushConstantParam("pingpong", &pingpong_);

            // horizontal dx
            fftHorizCompute_->updatePushConstantParam("offset", (void*)&dxOffset);
            fftHorizCompute_->updateGpuPush();
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, horizBundle, Resolution / 16, Resolution / 16, 1);

            // horizontal dy
            fftHorizCompute_->updatePushConstantParam("offset", (void*)&dyOffset);
            fftHorizCompute_->updateGpuPush();
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, horizBundle, Resolution / 16, Resolution / 16, 1);

            // horizontal dz
            fftHorizCompute_->updatePushConstantParam("offset", (void*)&dzOffset);
            fftHorizCompute_->updateGpuPush();
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, horizBundle, Resolution / 16, Resolution / 16, 1);
        }

        vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);

        // dispatch vertical fft
        for (size_t n = 0; n < log2N_; ++n)
        {
            pingpong_ = pingpong_ == 0 ? 1 : 0;
            fftVertCompute_->updatePushConstantParam("stage", &n);
            fftVertCompute_->updatePushConstantParam("pingpong", &pingpong_);

            // vertical dx
            fftVertCompute_->updatePushConstantParam("offset", (void*)&dxOffset);
            fftVertCompute_->updateGpuPush();
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, vertBundle, Resolution / 16, Resolution / 16, 1);

            // vertical dy
            fftVertCompute_->updatePushConstantParam("offset", (void*)&dyOffset);
            fftVertCompute_->updateGpuPush();
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, vertBundle, Resolution / 16, Resolution / 16, 1);

            // vertical dz
            fftVertCompute_->updatePushConstantParam("offset", (void*)&dzOffset);
            fftVertCompute_->updateGpuPush();
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, vertBundle, Resolution / 16, Resolution / 16, 1);
        }
    });

    rGraph.addExecutorPass("displacement", [=](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        if (pingpong_)
        {
            displaceCompute_->copySsbo(
                *fftVertCompute_,
                0,
                0,
                StorageBuffer::AccessType::ReadWrite,
                "SsboBufferA",
                "ssbo");
        }
        else
        {
            displaceCompute_->copySsbo(
                *fftVertCompute_,
                1,
                1,
                StorageBuffer::AccessType::ReadWrite,
                "SsboBufferA",
                "ssbo");
        }

        displaceCompute_->addStorageImage(
            driver,
            "DisplacementMap",
            fftOutputImage_->getBackendHandle(),
            0,
            ImageStorageSet::StorageType::WriteOnly);
        displaceCompute_->addStorageImage(
            driver,
            "HeightMap",
            heightMap_->getBackendHandle(),
            1,
            ImageStorageSet::StorageType::WriteOnly);

        displaceCompute_->addUboParam("N", backend::BufferElementType::Float, (void*)&N);
        displaceCompute_->addUboParam(
            "choppyFactor", backend::BufferElementType::Float, (void*)&options.choppyFactor);
        displaceCompute_->addUboParam(
            "offset_dx", backend::BufferElementType::Int, (void*)&dxOffset);
        displaceCompute_->addUboParam(
            "offset_dy", backend::BufferElementType::Int, (void*)&dyOffset);
        displaceCompute_->addUboParam(
            "offset_dz", backend::BufferElementType::Int, (void*)&dzOffset);

        auto* bundle = displaceCompute_->build(engine_, "fft_displacement.comp");

        vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);
        driver.dispatchCompute(
            cmds.getCmdBuffer().cmdBuffer, bundle, Resolution / 16, Resolution / 16, 1);
    });

    rGraph.addExecutorPass("generate_maps", [=](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        // input samplers
        genMapCompute_->addImageSampler(
            driver,
            "fftOutputImage",
            fftOutputImage_->getBackendHandle(),
            0,
            {backend::SamplerFilter::Nearest});
        genMapCompute_->addImageSampler(
            driver,
            "HeightMap",
            heightMap_->getBackendHandle(),
            1,
            {backend::SamplerFilter::Nearest});

        // output storage images
        genMapCompute_->addStorageImage(
            driver,
            "DisplacementMap",
            displacementMap_->getBackendHandle(),
            2,
            ImageStorageSet::StorageType::WriteOnly);
        genMapCompute_->addStorageImage(
            driver,
            "GradientMap",
            gradientMap_->getBackendHandle(),
            3,
            ImageStorageSet::StorageType::WriteOnly);

        genMapCompute_->addUboParam("N", backend::BufferElementType::Float, (void*)&N);
        genMapCompute_->addUboParam(
            "choppyFactor", backend::BufferElementType::Float, (void*)&options.choppyFactor);
        genMapCompute_->addUboParam(
            "gridLength", backend::BufferElementType::Float, (void*)&options.gridLength);

        auto* bundle = genMapCompute_->build(engine_, "generate_maps.comp");

        vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);
        driver.dispatchCompute(
            cmds.getCmdBuffer().cmdBuffer, bundle, Resolution / 16, Resolution / 16, 1);

        cmds.flush();
    });
}

// ============================ client api =================================

WaveGenerator::WaveGenerator() = default;
WaveGenerator::~WaveGenerator() = default;

} // namespace yave