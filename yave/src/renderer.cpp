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

#include "renderer.h"

#include "colour_pass.h"
#include "engine.h"
#include "mapped_texture.h"
#include "post_process.h"
#include "render_graph/resources.h"
#include "scene.h"
#include "skybox.h"
#include "wave_generator.h"
#include "utility/cstring.h"
#include "vulkan-api/renderpass.h"

#include <algorithm>

namespace yave
{

IRenderer::IRenderer(IEngine* engine) : engine_(engine), rGraph_(engine->driver())
{
    createBackbufferRT();
}
IRenderer::~IRenderer() {}

void IRenderer::createBackbufferRT() noexcept
{
    auto& driver = engine_->driver();
    vkapi::Swapchain* swapchain = engine_->getCurrentSwapchain();
    if (!swapchain)
    {
        return;
    }

    // create the backbuffer depth texture
    vk::Format depthFormat = driver.getSupportedDepthFormat();

    depthHandle_ = driver.createTexture2d(
        depthFormat,
        swapchain->extentsWidth(),
        swapchain->extentsHeight(),
        1,
        1,
        1,
        vk::ImageUsageFlagBits::eDepthStencilAttachment);

    // assume triple buffered backbuffer
    for (uint8_t idx = 0; idx < 3; ++idx)
    {
        auto scTextureHandle = swapchain->getTexture(idx);

        std::array<vkapi::RenderTarget::AttachmentInfo, vkapi::RenderTarget::MaxColourAttachCount>
            colourAttach;
        colourAttach[0] = {0, 0, scTextureHandle};
        vkapi::RenderTarget::AttachmentInfo depth {0, 0, depthHandle_};
        auto handle = driver.createRenderTarget(
            "backbuffer",
            swapchain->extentsWidth(),
            swapchain->extentsHeight(),
            false,
            1,
            {},
            colourAttach,
            depth,
            {});
        rtHandles_[idx] = handle;
    }
}

void IRenderer::beginFrameI() noexcept
{
    ASSERT_LOG(engine_);

    auto swapchain = engine_->getCurrentSwapchain();
    bool success = engine_->driver().beginFrame(*swapchain);
    // need to handle out of date swapchains here
    ASSERT_LOG(success);
}

void IRenderer::endFrameI() noexcept
{
    ASSERT_LOG(engine_);
    auto swapchain = engine_->getCurrentSwapchain();
    engine_->driver().endFrame(*swapchain);
}

void IRenderer::renderSingleSceneI(vkapi::VkDriver& driver, IScene* scene, RenderTarget& rTarget)
{
    auto& cmds = driver.getCommands();
    auto& cmdBuffer = cmds.getCmdBuffer().cmdBuffer;
    auto& queue = scene->getRenderQueue();

    vkapi::RenderPassData data;
    data.width = rTarget.getWidth();
    data.height = rTarget.getHeight();

    memcpy(
        data.loadClearFlags.data(),
        rTarget.getLoadFlags(),
        sizeof(backend::LoadClearFlags) * vkapi::RenderTarget::MaxAttachmentCount);
    memcpy(
        data.storeClearFlags.data(),
        rTarget.getStoreFlags(),
        sizeof(backend::StoreClearFlags) * vkapi::RenderTarget::MaxAttachmentCount);

    scene->update();

    driver.beginRenderpass(cmdBuffer, data, rTarget.getHandle());
    queue.render(*engine_, *scene, cmdBuffer, RenderQueue::Type::Colour);
    driver.endRenderpass(cmdBuffer);
}

void IRenderer::renderI(
    vkapi::VkDriver& driver, IScene* scene, float dt, util::Timer<NanoSeconds>& timer, bool clearSwap)
{
    rGraph_.reset();

    // ensure the post-process manager has been initialised
    engine_->getPostProcess()->init(*scene);

    // update the renderable objects and lights
    scene->update();

    // resource input which will be moved to the backbuffer RT
    rg::RenderGraphHandle input;

    auto* swapchain = engine_->getCurrentSwapchain();
    uint32_t imageIndex = driver.getCurrentPresentIndex();

    // import the backbuffer render target into the render graph
    rg::ImportedRenderTarget::Descriptor desc;
    desc.width = swapchain->extentsWidth();
    desc.height = swapchain->extentsHeight();
    desc.usage = vk::ImageUsageFlagBits::eColorAttachment;

    // store/clear flags for final colour attachment.
    desc.storeClearFlags[0] = backend::StoreClearFlags::Store;
    desc.loadClearFlags[0] =
        clearSwap ? backend::LoadClearFlags::Clear : backend::LoadClearFlags::Load;
    desc.finalLayouts[0] = vk::ImageLayout::ePresentSrcKHR;
    desc.initialLayouts[0] =
        clearSwap ? vk::ImageLayout::eUndefined : vk::ImageLayout::ePresentSrcKHR;

    // should be definable via the client api
    desc.clearColour = {0.0f, 0.0f, 0.0f, 1.0f};

    auto backbufferRT = rGraph_.importRenderTarget("backbuffer", desc, rtHandles_[imageIndex]);

    vk::Format depthFormat = driver.getSupportedDepthFormat();

    // post process options
    BloomOptions& bloomOptions = scene->getBloomOptions();
    GbufferOptions& gbufferOptions = scene->getGbufferOptions();

    if (!scene->withPostProcessing())
    {
        bloomOptions.enabled = false;
    }

    // upate the wave data - the actual draw happens in the material queue
    IWaveGenerator* waveGen = scene->getWaveGenerator();
    if (waveGen)
    {
        waveGen->render(rGraph_, *scene, dt, timer);
    }

    // fill the gbuffers - this can't be the final render target unless gbuffers are disabled due
    // to the gBuffers requiring resolviong down to a single render target in the lighting pass
    input = ColourPass::render(
        *engine_, *scene, rGraph_, gbufferOptions.width, gbufferOptions.height, depthFormat);

    ILightManager* lightManager = engine_->getLightManagerI();
    PostProcess* pp = engine_->getPostProcess();

    if (scene->withGbuffer())
    {
        input = lightManager->render(rGraph_, *scene, desc.width, desc.height, depthFormat);
    }

    // post-process stage
    if (bloomOptions.enabled)
    {
        input = pp->bloom(rGraph_, desc.width, desc.height, bloomOptions, dt);
    }

    rGraph_.moveResource(input, backbufferRT);
    rGraph_.addPresentPass(backbufferRT);

    // now compile and execute the frame graph
    rGraph_.compile();

#ifndef NDEBUG
    std::string output;
    rGraph_.getDependencyGraph().exportGraphViz(output);
    FILE* fp = fopen("C:/Users/THEGM/Desktop/render-graph.dot", "w");
    fprintf(fp, "%s", output.c_str());
    fclose(fp);
#endif

    rGraph_.execute();
}

void IRenderer::shutDown(vkapi::VkDriver& driver) noexcept {}

// ================== client api =====================

Renderer::Renderer() {}
Renderer::~Renderer() {}

void IRenderer::beginFrame() { beginFrameI(); }

void IRenderer::endFrame() { endFrameI(); }

void IRenderer::render(
    Engine* engine, Scene* scene, float dt, util::Timer<NanoSeconds>& timer, bool clearSwap)
{
    renderI(
        reinterpret_cast<IEngine*>(engine)->driver(),
        reinterpret_cast<IScene*>(scene),
        dt,
        timer,
        clearSwap);
};

void IRenderer::renderSingleScene(Engine* engine, Scene* scene, RenderTarget& rTarget)
{
    renderSingleSceneI(
        reinterpret_cast<IEngine*>(engine)->driver(), reinterpret_cast<IScene*>(scene), rTarget);
}

void RenderTarget::setColourTexture(Texture* tex, uint8_t attachIdx)
{
    ASSERT_FATAL(
        attachIdx < MaxAttachCount,
        "Attachment index of %d is greater than the max allowed value %d",
        attachIdx,
        MaxAttachCount);
    ASSERT_FATAL(tex, "The target resource must be valid.");
    attachments_[attachIdx].texture = tex;
}

void RenderTarget::setDepthTexture(Texture* tex)
{
    ASSERT_FATAL(tex, "The target resource must be valid.");
    attachments_[DepthAttachIdx].texture = tex;
}

void RenderTarget::setMipLevel(uint8_t level, uint8_t attachIdx)
{
    ASSERT_FATAL(
        attachIdx < MaxAttachCount,
        "Attachment index of %d is greater than the max allowed value %d",
        attachIdx,
        MaxAttachCount);
    attachments_[attachIdx].mipLevel = level;
}

void RenderTarget::setLayer(uint8_t layer, uint8_t attachIdx)
{
    ASSERT_FATAL(
        attachIdx < MaxAttachCount,
        "Attachment index of %d is greater than the max allowed value %d",
        attachIdx,
        MaxAttachCount);
    attachments_[attachIdx].layer = layer;
}

void RenderTarget::setLoadFlags(backend::LoadClearFlags flags, uint8_t attachIdx)
{
    ASSERT_FATAL(
        attachIdx < MaxAttachCount,
        "Attachment index of %d is greater than the max allowed value %d",
        attachIdx,
        MaxAttachCount);
    loadFlags_[attachIdx] = flags;
}

void RenderTarget::setStoreFlags(backend::StoreClearFlags flags, uint8_t attachIdx)
{
    ASSERT_FATAL(
        attachIdx < MaxAttachCount,
        "Attachment index of %d is greater than the max allowed value %d",
        attachIdx,
        MaxAttachCount);
    storeFlags_[attachIdx] = flags;
}

void RenderTarget::build(Engine* engine, const util::CString& name, bool multiView)
{
    ASSERT_FATAL(
        attachments_[0].texture || attachments_[DepthAttachIdx].texture,
        "Render target must contain either a valid colour or depth attachment.");

    IEngine* iengine = reinterpret_cast<IEngine*>(engine);
    auto& driver = iengine->driver();

    // convert attachment information to vulkan api format
    vkapi::RenderTarget vkRt;
    vkRt.samples = samples_;

    uint32_t minWidth = std::numeric_limits<uint32_t>::max();
    uint32_t minHeight = std::numeric_limits<uint32_t>::max();
    uint32_t maxWidth = 0;
    uint32_t maxHeight = 0;

    for (int i = 0; i < vkapi::RenderTarget::MaxColourAttachCount; ++i)
    {
        if (attachments_[i].texture)
        {
            IMappedTexture* mT = reinterpret_cast<IMappedTexture*>(attachments_[i].texture);

            vkRt.colours[i].handle = mT->getBackendHandle();
            vkRt.colours[i].level = attachments_[i].mipLevel;
            vkRt.colours[i].layer = attachments_[i].layer;

            uint32_t width = mT->getWidth() >> attachments_[i].mipLevel;
            uint32_t height = mT->getHeight() >> attachments_[i].mipLevel;
            minWidth = std::min(minWidth, width);
            minHeight = std::min(minHeight, height);
            maxWidth = std::max(maxWidth, width);
            maxHeight = std::max(maxHeight, height);
        }
    }
    width_ = minWidth;
    height_ = minHeight;

    if (attachments_[DepthAttachIdx].texture)
    {
        IMappedTexture* dT =
            reinterpret_cast<IMappedTexture*>(attachments_[DepthAttachIdx].texture);
        vkRt.depth.handle = dT->getBackendHandle();
        vkRt.depth.level = attachments_[DepthAttachIdx].mipLevel;
    }

    handle_ = driver.createRenderTarget(
        name, minWidth, minHeight, multiView, samples_, clearCol_, vkRt.colours, vkRt.depth, {});
}

} // namespace yave
