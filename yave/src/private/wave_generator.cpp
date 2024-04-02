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
#include "index_buffer.h"
#include "managers/renderable_manager.h"
#include "mapped_texture.h"
#include "object_manager.h"
#include "renderable.h"
#include "scene.h"
#include "vertex_buffer.h"
#include "yave/texture_sampler.h"

#include <random>

namespace yave
{

IWaveGenerator::IWaveGenerator(IEngine& engine, IScene& scene)
    : engine_(engine), log2N_(std::log(Resolution) / log(2)), pingpong_(0), updateSpectrum_(true)
{
    auto initSpecShaderCode = vkapi::ShaderProgramBundle::loadShader("initial_spectrum.comp");
    initialSpecCompute_ = std::make_unique<Compute>(engine, initSpecShaderCode);

    auto specShaderCode = vkapi::ShaderProgramBundle::loadShader("fft_spectrum.comp");
    specCompute_ = std::make_unique<Compute>(engine, specShaderCode);

    auto butterflyShaderCode = vkapi::ShaderProgramBundle::loadShader("fft_butterfly.comp");
    butterflyCompute_ = std::make_unique<Compute>(engine, butterflyShaderCode);

    auto fftHorizShaderCode = vkapi::ShaderProgramBundle::loadShader("fft_horiz.comp");
    fftHorizCompute_ = std::make_unique<Compute>(engine, fftHorizShaderCode);

    auto fftVertShaderCode = vkapi::ShaderProgramBundle::loadShader("fft_vert.comp");
    fftVertCompute_ = std::make_unique<Compute>(engine, fftVertShaderCode);

    auto displaceShaderCode = vkapi::ShaderProgramBundle::loadShader("fft_displacement.comp");
    displaceCompute_ = std::make_unique<Compute>(engine, displaceShaderCode);

    auto genmapShaderCode = vkapi::ShaderProgramBundle::loadShader("generate_maps.comp");
    genMapCompute_ = std::make_unique<Compute>(engine, genmapShaderCode);

    auto reverse = [this](int idx) -> uint32_t {
        uint32_t res = 0;
        for (size_t i = 0; i < log2N_; ++i)
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
    noiseTexture_ = engine.createMappedTexture();
    noiseTexture_->setTexture(
        noiseMap_,
        Resolution * Resolution * 4 * sizeof(float),
        Resolution,
        Resolution,
        1,
        1,
        Texture::TextureFormat::RGBA32F,
        backend::ImageUsage::Storage);

    butterflyLut_ = engine.createMappedTexture();
    butterflyLut_->setEmptyTexture(
        log2N_, Resolution, Texture::TextureFormat::RGBA32F, backend::ImageUsage::Storage, 1, 1);

    // output textures for h0k and h0-k
    h0kTexture_ = engine.createMappedTexture();
    h0minuskTexture_ = engine.createMappedTexture();
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
    fftOutputImage_ = engine_.createMappedTexture();
    fftOutputImage_->setEmptyTexture(
        Resolution,
        Resolution,
        Texture::TextureFormat::RG32F,
        backend::ImageUsage::Storage | backend::ImageUsage::Sampled,
        1,
        1);
    heightMap_ = engine_.createMappedTexture();
    heightMap_->setEmptyTexture(
        Resolution,
        Resolution,
        Texture::TextureFormat::R32F,
        backend::ImageUsage::Storage | backend::ImageUsage::Sampled,
        1,
        1);
    normalMap_ = engine_.createMappedTexture();
    normalMap_->setEmptyTexture(
        Resolution,
        Resolution,
        Texture::TextureFormat::RG32F,
        backend::ImageUsage::Storage | backend::ImageUsage::Sampled,
        1,
        1);

    // map generation
    displacementMap_ = engine_.createMappedTexture();
    displacementMap_->setEmptyTexture(
        Resolution,
        Resolution,
        Texture::TextureFormat::RGBA32F,
        backend::ImageUsage::Storage | backend::ImageUsage::Sampled,
        1,
        1);
    gradientMap_ = engine_.createMappedTexture();
    gradientMap_->setEmptyTexture(
        Resolution,
        Resolution,
        Texture::TextureFormat::RGBA32F,
        backend::ImageUsage::Storage | backend::ImageUsage::Sampled,
        1,
        1);

    // create the material objects
    IRenderableManager* rm = engine_.getRenderableManager();
    IObjectManager* om = engine_.getObjManager();
    waterObj_ = om->createObject();
    scene.addObject(waterObj_);

    material_ = rm->createMaterial();

    // create the vertices for the tesselation patch
    // NOTE: The patch size cannot be changed during runtime at present
    generatePatch();

    buildMaterial(scene);
}

IWaveGenerator::~IWaveGenerator() = default;

void IWaveGenerator::generatePatch() noexcept
{
    patchVertices.reserve(options.patchCount * options.patchCount * 5);

    int patchCount = static_cast<int>(options.patchCount);
    float width = 10.0f;
    float height = 10.0f;

    // interleaved pos and uv data for tesselation patch
    // Note: The y-axis for position is calculated from the height map on the tesselation shader
    for (size_t y = 0; y < options.patchCount; ++y)
    {
        for (size_t x = 0; x < options.patchCount; ++x)
        {
            patchVertices.push_back(
                static_cast<float>(x) * width + width / 2.0f -
                static_cast<float>(options.patchCount) * width / 2.0f);
            patchVertices.push_back(0.0f);
            patchVertices.push_back(
                static_cast<float>(y) * height + height / 2.0f -
                static_cast<float>(options.patchCount) * height / 2.0f);
            patchVertices.push_back(
                static_cast<float>(x) / static_cast<float>(options.patchCount - 1));
            patchVertices.push_back(
                static_cast<float>(y) / static_cast<float>(options.patchCount - 1));
        }
    }

    // indices
    const uint32_t w = (patchCount - 1);
    patchIndices.resize(w * w * 4);

    for (uint32_t x = 0; x < w; ++x)
    {
        for (uint32_t y = 0; y < w; ++y)
        {
            uint32_t index = (x + y * w) * 4;
            patchIndices[index] = (x + y * options.patchCount);
            patchIndices[index + 1] = patchIndices[index] + options.patchCount;
            patchIndices[index + 2] = patchIndices[index + 1] + 1;
            patchIndices[index + 3] = patchIndices[index] + 1;
        }
    }
}

void IWaveGenerator::buildMaterial(IScene& scene)
{
    auto& driver = engine_.driver();
    IRenderableManager* rm = engine_.getRenderableManager();

    TextureSampler sampler(
        backend::SamplerFilter::Linear,
        backend::SamplerFilter::Linear,
        backend::SamplerAddressMode::ClampToEdge,
        16);

    // tesselation evaluation shader
    mathfu::vec2 viewportDim {
        (float)engine_.getCurrentSwapchain()->extentsWidth(),
        (float)engine_.getCurrentSwapchain()->extentsHeight()};
    material_->addUboParam(
        "tessEdgeSize",
        backend::BufferElementType::Float,
        1,
        backend::ShaderStage::TesselationCon,
        &options.tessEdgeSize);
    material_->addUboParam(
        "tessFactor",
        backend::BufferElementType::Float,
        1,
        backend::ShaderStage::TesselationCon,
        &options.tessFactor);
    material_->addUboParam(
        "screenSize",
        backend::BufferElementType::Float2,
        1,
        backend::ShaderStage::TesselationCon,
        &viewportDim);

    // tesselation control shader
    material_->addImageTexture(
        "DisplacementMap",
        engine_.driver(),
        displacementMap_->getBackendHandle(),
        backend::ShaderStage::TesselationEval,
        sampler.get(),
        0);
    material_->addUboParam(
        "dispFactor",
        backend::BufferElementType::Float,
        1,
        backend::ShaderStage::TesselationEval,
        &options.dispFactor);

    // fragment shader
    material_->addImageTexture(
        "GradientMap",
        engine_.driver(),
        displacementMap_->getBackendHandle(),
        backend::ShaderStage::Fragment,
        sampler.get(),
        0);
    material_->addImageTexture(
        "NormalMap",
        engine_.driver(),
        normalMap_->getBackendHandle(),
        backend::ShaderStage::Fragment,
        sampler.get(),
        1);

    IRenderable* render = engine_.createRenderable();
    IVertexBuffer* vBuffer = engine_.createVertexBuffer();
    IIndexBuffer* iBuffer = engine_.createIndexBuffer();
    IRenderPrimitive* prim = engine_.createRenderPrimitive();
    render->setPrimitiveCount(1);
    render->skipVisibilityChecks();

    size_t verticesCount = patchVertices.size();
    vBuffer->addAttribute(
        util::ecast(VertexBuffer::BindingType::Position), backend::BufferElementType::Float3);
    vBuffer->addAttribute(
        util::ecast(VertexBuffer::BindingType::Uv), backend::BufferElementType::Float2);
    vBuffer->build(driver, verticesCount * sizeof(float), patchVertices.data());
    iBuffer->build(
        driver, patchIndices.size(), patchIndices.data(), backend::IndexBufferType::Uint32);
    prim->addMeshDrawData(patchIndices.size(), 0, 0);

    prim->setVertexBuffer(vBuffer);
    prim->setIndexBuffer(iBuffer);
    prim->setTopology(backend::PrimitiveTopology::PatchList);
    render->setPrimitive(prim, 0);
    render->setTesselationVertCount(4);

    material_->setCullMode(backend::cullModeToVk(backend::CullMode::Back));
    material_->setViewLayer(0x3);
    prim->setMaterial(material_);

    rm->build(scene, render, waterObj_, {}, "water.glsl");
}

void IWaveGenerator::updateCompute(
    rg::RenderGraph& rGraph, IScene& scene, float dt, util::Timer<NanoSeconds>& timer)
{
    auto N = static_cast<float>(Resolution);
    auto log2N = static_cast<float>(log2N_);

    // only generate the initial spectrum data if something has changed - i.e wind speed or
    // direction
    if (updateSpectrum_)
    {
        rGraph.addExecutorPass("initial_spectrum", [=](vkapi::VkDriver& driver) {
            auto& cmds = driver.getCommands();

            initialSpecCompute_->addStorageImage(
                "NoiseImage",
                noiseTexture_->getBackendHandle(),
                0,
                ImageStorageSet::StorageType::ReadOnly);

            // the output textures - h0k and h0-k
            initialSpecCompute_->addStorageImage(
                "H0kImage",
                h0kTexture_->getBackendHandle(),
                1,
                ImageStorageSet::StorageType::WriteOnly);
            initialSpecCompute_->addStorageImage(
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

            auto* bundle = initialSpecCompute_->build(engine_);
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, bundle, Resolution / 16, Resolution / 16, 1);
        });

        // Note: the butterfly image only needs updating if user defined changes in resolution are
        // allowed at some point. This may need moving under its own flag.
        rGraph.addExecutorPass("fft_butterfly", [=](vkapi::VkDriver& driver) {
            auto& cmds = driver.getCommands();

            butterflyCompute_->addStorageImage(
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

            auto* bundle = butterflyCompute_->build(engine_);
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, bundle, log2N_, Resolution / 16, 1);
        });

        updateSpectrum_ = false;
    }

    rGraph.addExecutorPass("spectrum", [=, &timer](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        // input images from the initial spectrum compute call
        specCompute_->addStorageImage(
            "H0kImage", h0kTexture_->getBackendHandle(), 0, ImageStorageSet::StorageType::ReadOnly);
        specCompute_->addStorageImage(
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

        float time = static_cast<float>(timer.getTimeElapsed()) / static_cast<float>(1'000'000'000);

        specCompute_->addUboParam("N", backend::BufferElementType::Int, (void*)&Resolution);
        specCompute_->addUboParam("L", backend::BufferElementType::Int, (void*)&options.L);
        specCompute_->addUboParam("time", backend::BufferElementType::Float, (void*)&time);
        specCompute_->addUboParam("offset_dx", backend::BufferElementType::Int, (void*)&dxOffset);
        specCompute_->addUboParam("offset_dy", backend::BufferElementType::Int, (void*)&dyOffset);
        specCompute_->addUboParam("offset_dz", backend::BufferElementType::Int, (void*)&dzOffset);

        auto* bundle = specCompute_->build(engine_);

        vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);
        driver.dispatchCompute(
            cmds.getCmdBuffer().cmdBuffer, bundle, Resolution / 16, Resolution / 16, 1);
    });

    rGraph.addExecutorPass("fft", [=](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        // setup horizontal fft
        fftHorizCompute_->addStorageImage(
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

        auto* horizBundle = fftHorizCompute_->build(engine_);

        // setup vertical fft
        fftVertCompute_->addStorageImage(
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

        auto* vertBundle = fftVertCompute_->build(engine_);
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
            vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);

            // horizontal dy
            fftHorizCompute_->updatePushConstantParam("offset", (void*)&dyOffset);
            fftHorizCompute_->updateGpuPush();
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, horizBundle, Resolution / 16, Resolution / 16, 1);
            vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);

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
            vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);

            // vertical dy
            fftVertCompute_->updatePushConstantParam("offset", (void*)&dyOffset);
            fftVertCompute_->updateGpuPush();
            driver.dispatchCompute(
                cmds.getCmdBuffer().cmdBuffer, vertBundle, Resolution / 16, Resolution / 16, 1);
            vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);

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
            "DisplacementMap",
            fftOutputImage_->getBackendHandle(),
            0,
            ImageStorageSet::StorageType::WriteOnly);
        displaceCompute_->addStorageImage(
            "HeightMap",
            heightMap_->getBackendHandle(),
            1,
            ImageStorageSet::StorageType::WriteOnly);
        displaceCompute_->addStorageImage(
            "NormalMap",
            normalMap_->getBackendHandle(),
            2,
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

        auto* bundle = displaceCompute_->build(engine_);

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
            "DisplacementMap",
            displacementMap_->getBackendHandle(),
            2,
            ImageStorageSet::StorageType::WriteOnly);
        genMapCompute_->addStorageImage(
            "GradientMap",
            gradientMap_->getBackendHandle(),
            3,
            ImageStorageSet::StorageType::WriteOnly);

        genMapCompute_->addUboParam("N", backend::BufferElementType::Float, (void*)&N);
        genMapCompute_->addUboParam(
            "choppyFactor", backend::BufferElementType::Float, (void*)&options.choppyFactor);
        genMapCompute_->addUboParam(
            "gridLength", backend::BufferElementType::Float, (void*)&options.gridLength);

        auto* bundle = genMapCompute_->build(engine_);

        vkapi::VkContext::writeReadComputeBarrier(cmdBuffer);
        driver.dispatchCompute(
            cmds.getCmdBuffer().cmdBuffer, bundle, Resolution / 16, Resolution / 16, 1);

        cmds.flush();
    });
}

void IWaveGenerator::transitionImagesToShaderRead(rg::RenderGraph& rGraph)
{
    rGraph.addExecutorPass("transition_images_shader_read", [=](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        displacementMap_->getBackendHandle().getResource()->transition(
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            cmdBuffer,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eTessellationEvaluationShader);
        normalMap_->getBackendHandle().getResource()->transition(
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            cmdBuffer,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eFragmentShader);
        gradientMap_->getBackendHandle().getResource()->transition(
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            cmdBuffer,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eFragmentShader);
    });
}

void IWaveGenerator::transitionImagesToCompute(rg::RenderGraph& rGraph)
{
    rGraph.addExecutorPass("transition_images_compute", [=](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        displacementMap_->getBackendHandle().getResource()->transition(
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            cmdBuffer,
            vk::PipelineStageFlagBits::eTessellationEvaluationShader,
            vk::PipelineStageFlagBits::eComputeShader);
        normalMap_->getBackendHandle().getResource()->transition(
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            cmdBuffer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eComputeShader);
        gradientMap_->getBackendHandle().getResource()->transition(
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            cmdBuffer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eComputeShader);
    });
}

} // namespace yave