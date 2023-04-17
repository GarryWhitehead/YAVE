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

#include "render_graph_handle.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/renderpass.h"

namespace yave
{
namespace rg
{
// forward declerations
class RenderGraph;
class RenderPassNode;
class ResourceBase;

class RenderGraphResource
{
public:
    struct Info
    {
        vkapi::RenderPassData data;
        vkapi::RenderTargetHandle handle;
    };

    RenderGraphResource(RenderGraph& rGraph, RenderPassNode* node);

    /**
     * @brief Get the resource related to the given handle
     * @param handle A resource handle
     * @return The resource declared by the handle
     */
    ResourceBase* getResource(const RenderGraphHandle& handle) const noexcept;

    /**
     * @brief Get details of the specified render pass
     * @param A handle to a render target - this is returned by @p
     * createRenderTarget
     * @return A info object detailing the specifics of the renderpass
     */
    Info getRenderPassInfo(const RenderGraphHandle& handle) const noexcept;

    vkapi::TextureHandle getTextureHandle(const RenderGraphHandle& handle) const noexcept;

private:
    RenderGraph& rGraph_;

    RenderPassNode* rPassNode_;
};


} // namespace rg
} // namespace yave
