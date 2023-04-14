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

#include "render_graph_builder.h"

#include "render_graph.h"
#include "render_graph_pass.h"
#include "render_pass_node.h"
#include "utility/logger.h"

namespace yave
{
namespace rg
{

RenderGraphBuilder::RenderGraphBuilder(RenderGraph* rGraph, PassNodeBase* node)
{
    rGraph_ = rGraph;
    passNode_ = reinterpret_cast<RenderPassNode*>(node);
}

RenderGraphHandle RenderGraphBuilder::createResource(
    const util::CString& name, const TextureResource::Descriptor& desc)
{
    std::unique_ptr<TextureResource> tex =
        std::make_unique<TextureResource>(name, desc);
    return rGraph_->addResource(std::move(tex));
}

RenderGraphHandle RenderGraphBuilder::createSubResource(
    const util::CString& name,
    const TextureResource::Descriptor& desc,
    const RenderGraphHandle& parent)
{
    std::unique_ptr<TextureResource> tex =
        std::make_unique<TextureResource>(name, desc);
    return rGraph_->addSubResource(std::move(tex), parent);
}

RenderGraphHandle RenderGraphBuilder::addReader(
    const RenderGraphHandle& handle, vk::ImageUsageFlags usage)
{
    return rGraph_->addRead(handle, passNode_, usage);
}

RenderGraphHandle RenderGraphBuilder::addWriter(
    const RenderGraphHandle& handle, vk::ImageUsageFlags usage)
{
    return rGraph_->addWrite(handle, passNode_, usage);
}

void RenderGraphBuilder::addSideEffect() noexcept
{
    passNode_->declareSideEffect();
}

RenderGraphHandle RenderGraphBuilder::createRenderTarget(
    const util::CString& name, const PassDescriptor& desc)
{
    return passNode_->createRenderTarget(name, desc);
}

void RenderGraphBuilder::addPresent(const RenderGraphHandle& presentHandle)
{
    rGraph_->addPresentPass(presentHandle);
}

} // namespace rg
} // namespace yave
