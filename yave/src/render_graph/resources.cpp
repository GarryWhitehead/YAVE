/* Copyright (c) 2018-2020 Garry Whitehead
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

#include "resources.h"

#include "render_graph/dependency_graph.h"
#include "render_graph/render_graph.h"
#include "render_graph/render_pass_node.h"
#include "render_graph/resource_node.h"
#include "vulkan-api/common.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/image.h"
#include "vulkan-api/texture.h"

namespace yave::rg
{

void ResourceBase::registerPass(PassNodeBase* node)
{
    readCount_++;
    // keep a record of the first and last passes to reference this resource
    if (!firstPassNode_)
    {
        firstPassNode_ = node;
    }
    lastPassNode_ = node;
}

bool ResourceBase::connectWriter(
    DependencyGraph& graph,
    PassNodeBase* passNode,
    ResourceNode* resourceNode,
    vk::ImageUsageFlags usage)
{
    ResourceEdge* edge = resourceNode->getWriterEdge(passNode);
    if (edge)
    {
        edge->usage_ |= usage;
    }
    else
    {
        auto newEdge = std::make_unique<ResourceEdge>(graph, passNode, resourceNode, usage);
        resourceNode->setWriterEdge(newEdge);
    }
    return true;
}

bool ResourceBase::connectReader(
    DependencyGraph& graph,
    PassNodeBase* passNode,
    ResourceNode* resourceNode,
    vk::ImageUsageFlags usage)
{
    ResourceEdge* edge = resourceNode->getReaderEdge(passNode);
    if (edge)
    {
        edge->usage_ |= usage;
    }
    else
    {
        auto newEdge = std::make_unique<ResourceEdge>(graph, resourceNode, passNode, usage);
        resourceNode->setReaderEdge(newEdge);
    }
    return true;
}

bool ResourceBase::isSubResource() const { return parent_ != this; }

TextureResource::TextureResource(const util::CString& name, const Descriptor& desc)
    : ResourceBase(name), desc_(desc)
{
}

void TextureResource::updateResourceUsage(
    DependencyGraph& graph,
    std::vector<std::unique_ptr<ResourceEdge>>& edges,
    ResourceEdge* writer) noexcept
{
    for (const auto& edge : edges)
    {
        if (graph.isValidEdge(reinterpret_cast<Edge*>(edge.get())))
        {
            imageUsage_ |= edge->usage_;
        }
    }
    if (writer)
    {
        imageUsage_ |= writer->usage_;
    }
    // also propogate the image usage flags to any subresources
    TextureResource* resource = this;
    while (resource != resource->getParent())
    {
        resource = dynamic_cast<TextureResource*>(resource->getParent());
        resource->imageUsage_ = imageUsage_;
    }
}

void TextureResource::bake(vkapi::VkDriver& driver)
{
    ASSERT_FATAL(imageUsage_, "Image usage not resolved for this resource!");
    handle_ = driver.createTexture2d(
        desc_.format, desc_.width, desc_.height, desc_.mipLevels, 1, 1, imageUsage_);
}

void TextureResource::destroy(vkapi::VkDriver& driver) { driver.destroyTexture2D(handle_); }

ImportedResource::ImportedResource(
    const util::CString& name,
    const TextureResource::Descriptor& desc,
    vk::ImageUsageFlags imageUsage,
    vkapi::TextureHandle handle)
    : TextureResource(name, desc)
{
    this->imageUsage_ = imageUsage;
    this->handle_ = handle;
}

void ImportedResource::bake(vkapi::VkDriver& driver)
{
    // not used for an imported resource
}

void ImportedResource::destroy(vkapi::VkDriver& driver)
{
    // not used for an imported resource
}

ImportedRenderTarget::ImportedRenderTarget(
    const util::CString& name,
    const vkapi::RenderTargetHandle& handle,
    const TextureResource::Descriptor& resDesc,
    const Descriptor& importedDesc)
    : ImportedResource(name, resDesc, importedDesc.usage, {}), rtHandle(handle), desc(importedDesc)
{
}

ImportedRenderTarget::~ImportedRenderTarget() = default;

} // namespace yave::rg
