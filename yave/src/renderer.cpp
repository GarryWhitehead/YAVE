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
#include "render_graph/resources.h"
#include "scene.h"
#include "skybox.h"
#include "vulkan-api/renderpass.h"

namespace yave
{

IRenderer::IRenderer(IEngine* engine)
    : engine_(engine), rGraph_(engine->driver())
{
    createBackbufferRT();
}
IRenderer::~IRenderer() {}

void IRenderer::createBackbufferRT() noexcept
{
    auto& driver = engine_->driver();
    auto swapchain = engine_->getCurrentSwapchain();

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

        std::array<
            vkapi::RenderTarget::AttachmentInfo,
            vkapi::RenderTarget::MaxColourAttachCount>
            colourAttach;
        colourAttach[0] = {0, 0, scTextureHandle};
        vkapi::RenderTarget::AttachmentInfo depth {0, 0, depthHandle_};
        auto handle = driver.createRenderTarget(
            "backbuffer",
            swapchain->extentsWidth(),
            swapchain->extentsHeight(),
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

void IRenderer::renderI(vkapi::VkDriver& driver, IScene& scene)
{
    rGraph_.reset();

    // TODO: This is a temp measure.
    uint32_t width = 1920;
    uint32_t height = 1080;
    
    // update the renderable objects and lights
    scene.update();

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
    desc.storeClearFlags[0] = vkapi::StoreClearFlags::Store;
    desc.loadClearFlags[0] = vkapi::LoadClearFlags::Clear;
    desc.finalLayouts[0] = vk::ImageLayout::ePresentSrcKHR;
    
    // should be definable via the client api
    desc.clearColour = {0.0f, 0.0f, 0.0f, 1.0f};

    auto backbufferRT =
        rGraph_.importRenderTarget("backbuffer", desc, rtHandles_[imageIndex]);

    vk::Format depthFormat = driver.getSupportedDepthFormat();

    // fill the GBuffers - this can't be the final render target due
    // to the gBuffers requiring resolviong down to a single render target
    ColourPass::render(*engine_, scene, rGraph_, width, height, depthFormat);

    ILightManager* lightManager = engine_->getLightManagerI();
    input = lightManager->render(rGraph_, desc.width, desc.height, depthFormat);

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

// ================== client api =====================

Renderer::Renderer() {}
Renderer::~Renderer() {}

void IRenderer::beginFrame() 
{ 
    beginFrameI(); 
}

void IRenderer::endFrame() 
{
    endFrameI(); 
}

void IRenderer::render(Engine* engine, Scene* scene) 
{
    renderI(
        reinterpret_cast<IEngine*>(engine)->driver(),
        *(reinterpret_cast<IScene*>(scene)));
};

} // namespace yave
