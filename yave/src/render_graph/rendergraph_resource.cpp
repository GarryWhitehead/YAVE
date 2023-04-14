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

#include "rendergraph_resource.h"

#include "render_graph.h"
#include "render_pass_node.h"
#include "resource_node.h"
#include "resources.h"

namespace yave
{
namespace rg
{

RenderGraphResource::RenderGraphResource(
    RenderGraph& rGraph, RenderPassNode* node)
    : rGraph_(rGraph), rPassNode_(node)
{
}

ResourceBase*
RenderGraphResource::getResource(const RenderGraphHandle& handle) const noexcept
{
    ASSERT_LOG(handle);
    return rGraph_.getResource(handle);
}

RenderGraphResource::Info RenderGraphResource::getRenderPassInfo(
    const RenderGraphHandle& handle) const noexcept
{
    ASSERT_LOG(handle);
    RenderPassInfo ri = rPassNode_->getRenderTargetInfo(handle);
    return {ri.vkBackend.rPassData, ri.desc.vkBackend.rtHandle};
}

vkapi::TextureHandle
RenderGraphResource::getTextureHandle(const RenderGraphHandle& handle) const noexcept
{
    ASSERT_LOG(handle);
    auto* textureResource =
        reinterpret_cast<TextureResource*>(rGraph_.getResource(handle));
    ASSERT_FATAL(
        textureResource && textureResource->handle(),
        "Invalid handle for vkapi resource.");
    return textureResource->handle();
}

} // namespace rg
} // namespace yave
