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

#include "resource_node.h"

#include "render_graph.h"
#include "render_pass_node.h"

namespace yave::rg
{

ResourceNode::ResourceNode(
    RenderGraph& rGraph,
    const util::CString& name,
    const RenderGraphHandle& resource,
    const RenderGraphHandle& parent)
    : Node(name, rGraph.getDependencyGraph()), rGraph_(rGraph), resource_(resource), parent_(parent)
{
}

ResourceEdge* ResourceNode::getWriterEdge(PassNodeBase* node)
{
    // If a writer pass has been registered with this node and its the
    // same node, then return the edge.
    if (writerPass_ && writerPass_->fromId == node->getId())
    {
        return writerPass_.get();
    }
    return nullptr;
}

void ResourceNode::setWriterEdge(std::unique_ptr<ResourceEdge>& edge)
{
    ASSERT_FATAL(!writerPass_, "Only one writer per resource allowed.");
    writerPass_ = std::move(edge);
}

ResourceEdge* ResourceNode::getReaderEdge(PassNodeBase* node)
{
    auto iter = std::find_if(
        readerPasses_.begin(), readerPasses_.end(), [node](std::unique_ptr<ResourceEdge>& edge) {
            return edge->toId == node->getId();
        });
    if (iter != readerPasses_.end())
    {
        return iter->get();
    }
    return nullptr;
}

void ResourceNode::setReaderEdge(std::unique_ptr<ResourceEdge>& edge)
{
    readerPasses_.emplace_back(std::move(edge));
}

void ResourceNode::setParentReader(ResourceNode* parentNode)
{
    auto& dGraph = rGraph_.getDependencyGraph();
    if (!parentReadEdge_)
    {
        parentReadEdge_ = std::make_unique<Edge>(
            dGraph, reinterpret_cast<Node*>(this), reinterpret_cast<Node*>(parentNode));
    }
}

void ResourceNode::setParentWriter(ResourceNode* parentNode)
{
    auto& dGraph = rGraph_.getDependencyGraph();
    if (!parentWriteEdge_)
    {
        parentWriteEdge_ = std::make_unique<Edge>(
            dGraph, reinterpret_cast<Node*>(this), reinterpret_cast<Node*>(parentNode));
    }
}

bool ResourceNode::hasWriterPass() const { return writerPass_ != nullptr; }

bool ResourceNode::hasReaders() const { return !readerPasses_.empty(); }

bool ResourceNode::hasWriters()
{
    const auto& writers =
        rGraph_.getDependencyGraph().getWriterEdges(reinterpret_cast<Node*>(this));
    return !writers.empty();
}

void ResourceNode::bakeResource(ResourceBase* resource) noexcept
{
    resourcesToBake_.push_back(resource);
}

void ResourceNode::destroyResource(ResourceBase* resource) noexcept
{
    resourcesToDestroy_.push_back(resource);
}

void ResourceNode::bakeResources(vkapi::VkDriver& driver) noexcept
{
    for (ResourceBase* resource : resourcesToBake_)
    {
        resource->bake(driver);
    }
}

void ResourceNode::destroyResources(vkapi::VkDriver& driver) noexcept
{
    for (ResourceBase* resource : resourcesToBake_)
    {
        resource->destroy(driver);
    }
}

void ResourceNode::updateResourceUsage() noexcept
{
    ASSERT_FATAL(resource_, "No resource handle set for this node!");
    ResourceBase* res = rGraph_.getResource(resource_);
    auto& dGraph = rGraph_.getDependencyGraph();
    res->updateResourceUsage(dGraph, readerPasses_, writerPass_.get());
}

ResourceNode* ResourceNode::getParentNode() noexcept
{
    if (parent_)
    {
        return rGraph_.getResourceNode(parent_);
    }
    return nullptr;
}

void ResourceNode::setAliasResourceEdge(ResourceNode* alias) noexcept
{
    ASSERT_LOG(alias);
    aliasEdge_ = std::make_unique<Edge>(rGraph_.getDependencyGraph(), this, alias);
}

} // namespace yave::rg
