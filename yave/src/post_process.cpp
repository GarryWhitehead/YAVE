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

#include "post_process.h"

#include "compute.h"
#include "engine.h"
#include "managers/light_manager.h"
#include "managers/renderable_manager.h"
#include "mapped_texture.h"
#include "material.h"
#include "object_manager.h"
#include "render_graph/rendergraph_resource.h"
#include "render_primitive.h"
#include "renderable.h"
#include "yave/options.h"

#include <unordered_map>

namespace yave
{

PostProcess::PostProcess(IEngine& engine)
    : engine_(engine),
      averageLumLut_(nullptr),
      lumCompute_(std::make_unique<Compute>(engine_)),
      avgCompute_(std::make_unique<Compute>(engine_))
{
}
PostProcess::~PostProcess() {}

void PostProcess::init(IScene& scene)
{
    // at the moment if we have already initialised don't do it again -
    // though this is dependent on the current scene so we may which to
    // allow re-initialisation or change where materials are built
    if (materials_.size() > 0)
    {
        return;
    }

    IRenderableManager* rm = engine_.getRenderableManagerI();
    IObjectManager* om = engine_.getObjManagerI();

    // all of the post process components to register
    std::unordered_map<std::string, PpRegister> registry {
        {"bloom",
         {"bloom.glsl", {{"gamma", ElementType::Float}}, {"LuminanceAvgLut", "ColourSampler"}}}};

    // pre-build all of the materials required for the post-process renderpass stage
    // TODO: at the moment the material shaders are not updated which means that changes
    // in variants won't be acted upon (will there be many varinats for post-process though??)
    for (auto& [name, reg] : registry)
    {
        IMaterial* mat = rm->createMaterialI();
        mat->withDynamicMeshTransformUbo(false);

        IRenderable* render = engine_.createRenderableI();
        IRenderPrimitive* prim = engine_.createRenderPrimitiveI();
        Object obj = om->createObjectI();

        // add any ubos in the registry (only fragment shader)
        for (const auto& ubo : reg.uboElements)
        {
            mat->addUboParamI(
                ubo.name, ubo.type, ubo.arrayCount, backend::ShaderStage::Fragment, nullptr);
        }
        // add any samplers in the registry
        size_t binding = 0;
        for (const auto& element : reg.samplers)
        {
            mat->setSamplerParamI(element, binding++, SamplerSet::SamplerType::e2d);
        }

        render->setPrimitiveCount(1);
        render->skipVisibilityChecks();
        prim->addMeshDrawDataI(0, 0, 3);
        render->setPrimitive(prim, 0);
        prim->setMaterialI(mat);

        rm->buildI(scene, render, obj, {}, reg.shader, "post_process");
        materials_[name] = {mat, obj};
    }

    // texture for adaptive exposure
    averageLumLut_ = engine_.createMappedTextureI();
    uint32_t zero = 0;
    averageLumLut_->setTextureI(
        &zero,
        1,
        1,
        1,
        1,
        Texture::TextureFormat::R32F,
        backend::ImageUsage::Storage | backend::ImageUsage::Sampled);
}

PostProcess::PpMaterial PostProcess::getMaterial(const std::string& name)
{
    auto iter = materials_.find(name);
    ASSERT_FATAL(
        iter != materials_.end(),
        "Post process material %s not found in registry list.",
        name.c_str());
    ASSERT_LOG(iter->second.material);
    return iter->second;
}

rg::RenderGraphHandle PostProcess::bloom(
    rg::RenderGraph& rGraph, uint32_t width, uint32_t height, const BloomOptions& options, float dt)
{
    auto& driver = engine_.driver();

    auto* blackboard = rGraph.getBlackboard();
    auto& light = blackboard->get("light");
    auto& lightHandle = reinterpret_cast<rg::TextureResource*>(rGraph.getResource(light))->handle();

    // dynamic exposure calculations - TODO: make optional
    // first step is to create the luminance histogram bin values
    rGraph.addExecutorPass("luminance_compute", [=, &lightHandle](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        uint32_t totalWorkCount = width * height;

        // images need to be in VK_IMAGE_LAYOUT_GENERAL for use as image stores.
        // we also add a memory barrier to make sure the fragment shader has
        // finished before using the light image.
        lightHandle.getResource()->transition(
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            cmdBuffer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::PipelineStageFlagBits::eComputeShader);

        lumCompute_->addStorageImage(
            driver,
            "ColourSampler",
            lightHandle,
            0,
            ImageStorageSet::StorageType::ReadOnly);

       lumCompute_->addSsbo(
            "histogram",
            backend::BufferElementType::Uint,
            StorageBuffer::AccessType::ReadWrite,
            0,
            "output_ssbo",
            nullptr,
            totalWorkCount);

        lumCompute_->addUboParam(
            "minLuminanceLog", backend::BufferElementType::Float, (void*)&options.minLuminanceLog);
        lumCompute_->addUboParam(
            "invLuminanceRange",
            backend::BufferElementType::Float,
            (void*)&options.invLuminanceRange);

        auto* bundle = lumCompute_->build(engine_, "luminance.comp");
        driver.dispatchCompute(cmds.getCmdBuffer().cmdBuffer, bundle, totalWorkCount / 256, 1, 1);

        cmds.flush();
    });

    rGraph.addExecutorPass("averagelum_compute", [=, &lightHandle](vkapi::VkDriver& driver) {
        auto& cmds = driver.getCommands();
        auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

        float numPixels = static_cast<float>(width * height);

        // memory barrier to ensure the last compute shader has finished writing to the image
        // before we start reading from it.
        lightHandle.getResource()->memoryBarrier(
            cmdBuffer,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eShaderRead,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eComputeShader);

        avgCompute_->addStorageImage(
            driver,
            "ColourSampler",
            averageLumLut_->getBackendHandle(),
            0,
            ImageStorageSet::StorageType::ReadWrite);

         avgCompute_->copySsbo(
            *lumCompute_,
            0,
            0,
            StorageBuffer::AccessType::ReadWrite,
            "SsboBuffer",
            "input_ssbo");

        avgCompute_->addUboParam(
            "minLuminanceLog", backend::BufferElementType::Float, (void*)&options.minLuminanceLog);
        avgCompute_->addUboParam(
            "invLuminanceRange",
            backend::BufferElementType::Float,
            (void*)&options.invLuminanceRange);
        avgCompute_->addUboParam("numPixels", backend::BufferElementType::Float, (void*)&numPixels);
        avgCompute_->addUboParam("timeDelta", backend::BufferElementType::Float, (void*)&dt);

        auto* bundle = avgCompute_->build(engine_, "average_lum.comp");
        driver.dispatchCompute(
            cmds.getCmdBuffer().cmdBuffer, bundle, static_cast<int>(numPixels) / 256, 1, 1);

        cmds.flush();
    });

    auto& rg = rGraph.addPass<BloomData>(
        "BloomPP",
        [&](rg::RenderGraphBuilder& builder, BloomData& data) {
            auto* blackboard = rGraph.getBlackboard();

            // Get the resources from the colour pass
            auto light = blackboard->get("light");

            rg::TextureResource::Descriptor desc;
            desc.format = vk::Format::eR8G8B8A8Unorm;
            desc.width = width;
            desc.height = height;
            data.bloom = builder.createResource("bloom", desc);

            // inputs into the pass
            data.light = builder.addReader(light, vk::ImageUsageFlagBits::eSampled);

            // ouput writes
            data.bloom = builder.addWriter(data.bloom, vk::ImageUsageFlagBits::eColorAttachment);

            rg::PassDescriptor passDesc;
            passDesc.attachments.attach.colour[0] = data.bloom;
            data.rt = builder.createRenderTarget("bloomRT", passDesc);
        },
        [=, &lightHandle](
            vkapi::VkDriver& driver,
            const BloomData& data,
            const rg::RenderGraphResource& resources) {
            auto& cmds = driver.getCommands();
            vk::CommandBuffer cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

            lightHandle.getResource()->transition(
                vk::ImageLayout::eGeneral,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                cmdBuffer,
                vk::PipelineStageFlagBits::eComputeShader,
                vk::PipelineStageFlagBits::eFragmentShader);

            vkapi::TextureHandle lightTex = resources.getTextureHandle(data.light);
            IMaterial* mat = getMaterial("bloom").material;

            mat->addImageTexture(
                "LuminanceAvgLut",
                driver,
                averageLumLut_->getBackendHandle(),
                {backend::SamplerFilter::Nearest});
            mat->addImageTexture(
                "ColourSampler", driver, lightTex, {backend::SamplerFilter::Nearest});
            mat->updateUboParam("gamma", backend::ShaderStage::Fragment, (void*)&options.gamma);
            mat->update(engine_);

            const auto& info = resources.getRenderPassInfo(data.rt);

            driver.beginRenderpass(cmdBuffer, info.data, info.handle);
            driver.draw(cmds.getCmdBuffer().cmdBuffer, *mat->getProgram());
            driver.endRenderpass(cmdBuffer);
        });

    return rg.getData().bloom;
}

} // namespace yave