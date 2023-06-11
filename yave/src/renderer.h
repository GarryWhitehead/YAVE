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

#pragma once

#include "render_graph/render_graph.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/program_manager.h"
#include "vulkan-api/swapchain.h"
#include "yave/renderer.h"
#include "yave/scene.h"

#include <array>

namespace yave
{
// forward declerations
class IScene;
class IEngine;

class IRenderer : public Renderer
{
public:
    IRenderer(IEngine* engine);
    ~IRenderer();

    void shutDown(vkapi::VkDriver& driver) noexcept;

    void createBackbufferRT() noexcept;

    void beginFrameI() noexcept;

    void endFrameI() noexcept;

    void renderI(
        vkapi::VkDriver& driver,
        IScene* scene,
        float dt,
        util::Timer<NanoSeconds>& timer, bool clearSwap = true);

    // draws into a single render target
    void renderSingleSceneI(vkapi::VkDriver& driver, IScene* scene, RenderTarget& rTarget);

    // ================ client api =====================

    void beginFrame() override;
    void endFrame() override;
    void render(
        Engine* engine, Scene* scene, float dt, util::Timer<NanoSeconds>& timer, bool clearSwap) override;
    void renderSingleScene(Engine* engine, Scene* scene, RenderTarget& rTarget) override;

private:
    IEngine* engine_;

    rg::RenderGraph rGraph_;

    // render targets for the backbuffer
    std::array<vkapi::RenderTargetHandle, 3> rtHandles_;

    // keep track of the depth texture - set by createBackBufferRT
    vkapi::TextureHandle depthHandle_;
};

} // namespace yave
