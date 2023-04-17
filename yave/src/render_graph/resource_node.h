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

#include "dependency_graph.h"
#include "render_graph_pass.h"

#include <memory>

namespace yave
{
namespace rg
{
// forward declerations
class RenderGraph;
struct Edge;
class PassNodeBase;

class ResourceEdge : public Edge
{
public:
    ResourceEdge(DependencyGraph& dGraph, Node* from, Node* to, vk::ImageUsageFlags usage)
        : Edge(dGraph, from, to), usage_(usage)
    {
    }

    vk::ImageUsageFlags usage_;
};

class ResourceNode : public Node
{
public:
    ResourceNode(
        RenderGraph& rGraph,
        const util::CString& name,
        const RenderGraphHandle& resource,
        const RenderGraphHandle& parent);

    ResourceEdge* getWriterEdge(PassNodeBase* node);

    void setWriterEdge(std::unique_ptr<ResourceEdge>& edge);

    ResourceEdge* getReaderEdge(PassNodeBase* node);

    void setReaderEdge(std::unique_ptr<ResourceEdge>& edge);

    void setParentReader(ResourceNode* parentNode);

    void setParentWriter(ResourceNode* parentNode);

    void bakeResource(ResourceBase* resource) noexcept;

    void destroyResource(ResourceBase* resource) noexcept;

    void bakeResources(vkapi::VkDriver& driver) noexcept;

    void destroyResources(vkapi::VkDriver& driver) noexcept;

    void setAliasResourceEdge(ResourceNode* alias) noexcept;

    bool hasWriterPass() const;

    bool hasReaders() const;

    bool hasWriters();

    ResourceNode* getParentNode() noexcept;

    void updateResourceUsage() noexcept;

    // ===================== getters =====================
    const RenderGraphHandle& resourceHandle() const { return resource_; }

private:
    RenderGraph& rGraph_;
    RenderGraphHandle resource_;
    RenderGraphHandle parent_;

    /// the pass which writes to this resource
    std::unique_ptr<ResourceEdge> writerPass_;

    std::unique_ptr<Edge> parentReadEdge_;
    std::unique_ptr<Edge> parentWriteEdge_;

    std::unique_ptr<Edge> aliasEdge_;

    std::vector<std::unique_ptr<ResourceEdge>> readerPasses_;

    // set when running compile()
    std::vector<ResourceBase*> resourcesToBake_;
    std::vector<ResourceBase*> resourcesToDestroy_;
};

} // namespace rg
} // namespace yave
