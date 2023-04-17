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

#include "render_pass_node.h"

#include "render_graph.h"
#include "render_graph_pass.h"
#include "rendergraph_resource.h"
#include "resource_node.h"
#include "resources.h"
#include "utility/assertion.h"
#include "utility/logger.h"
#include "vulkan-api/driver.h"

namespace yave
{
namespace rg
{

void RenderPassInfo::bake(const RenderGraph& rGraph)
{
    // imported targets declare their own info so nothing to do here.
    if (imported)
    {
        return;
    }

    std::array<vkapi::RenderTarget::AttachmentInfo, vkapi::RenderTarget::MaxColourAttachCount>
        colourInfo;
    for (uint8_t i = 0; i < colourInfo.size(); ++i)
    {
        if (desc.attachments.attachArray[i])
        {
            TextureResource* texture = reinterpret_cast<TextureResource*>(
                rGraph.getResource(desc.attachments.attachArray[i]));
            colourInfo[i].handle = texture->handle();
            ASSERT_FATAL(texture->handle(), "Invalid handle for colour attchment at index %d.", i);
            // also fill the layer and level from the subresource of the texture

            // now we have resolved the image usage - work out what to transition
            // to in the final layout of the renderpass
            if (texture->imageUsage_ & vk::ImageUsageFlagBits::eSampled ||
                texture->imageUsage_ & vk::ImageUsageFlagBits::eInputAttachment)
            {
                vkBackend.rPassData.finalLayouts[i] = vk::ImageLayout::eShaderReadOnlyOptimal;
            }
            else
            {
                // safe to assume that this is a colour attachment if not sampled/input??
                vkBackend.rPassData.finalLayouts[i] = vk::ImageLayout::eColorAttachmentOptimal;
            }
        }
    }

    std::array<vkapi::RenderTarget::AttachmentInfo, 2> depthStencilInfo;
    for (uint8_t i = 0; i < depthStencilInfo.size(); ++i)
    {
        auto& attachment =
            desc.attachments.attachArray[vkapi::RenderTarget::MaxColourAttachCount + i];
        if (attachment)
        {
            TextureResource* texture =
                reinterpret_cast<TextureResource*>(rGraph.getResource(attachment));
            depthStencilInfo[i].handle = texture->handle();
            ASSERT_FATAL(texture->handle(), "Invalid handle for depth/stencil attchment.");
            // also fill the layer and level from the subresource of the texture
        }
    }

    auto& driver = rGraph.driver();
    desc.vkBackend.rtHandle = driver.createRenderTarget(
        name,
        vkBackend.rPassData.width,
        vkBackend.rPassData.height,
        desc.samples,
        desc.clearColour,
        colourInfo,
        depthStencilInfo[0],
        depthStencilInfo[1]);
}

PassNodeBase::PassNodeBase(RenderGraph& rGraph, const util::CString& name)
    : Node(name, rGraph.getDependencyGraph()), rGraph_(rGraph)
{
}
PassNodeBase::~PassNodeBase() {}

void PassNodeBase::addToBakeList(ResourceBase* res)
{
    ASSERT_LOG(res);
    resourcesToBake_.emplace_back(res);
}

void PassNodeBase::addToDestroyList(ResourceBase* res)
{
    ASSERT_LOG(res);
    resourcesToDestroy_.emplace_back(res);
}

void PassNodeBase::bakeResourceList(vkapi::VkDriver& driver)
{
    for (ResourceBase* res : resourcesToBake_)
    {
        res->bake(driver);
    }
}

void PassNodeBase::destroyResourceList(vkapi::VkDriver& driver)
{
    for (ResourceBase* res : resourcesToDestroy_)
    {
        res->destroy(driver);
    }
}

void PassNodeBase::addResource(const RenderGraphHandle& handle)
{
    ResourceBase* resource = rGraph_.getResource(handle);
    resource->registerPass(this);
    resourceHandles_.push_back(handle);
}

// ====================== RenderPass Node ====================================

RenderPassNode::RenderPassNode(
    RenderGraph& rGraph, RenderGraphPassBase* rgPass, const util::CString& name)
    : PassNodeBase(rGraph, name), rgPass_(rgPass)
{
}

RenderGraphHandle
RenderPassNode::createRenderTarget(const util::CString& name, const PassDescriptor& desc)
{
    RenderPassInfo info;
    info.name = name;
    auto& depGraph = rGraph_.getDependencyGraph();
    auto writerEdges = depGraph.getWriterEdges(this);
    auto readerEdges = depGraph.getReaderEdges(this);

    ASSERT_FATAL(
        desc.attachments.attach.colour[0],
        "At least one colour attachment must be declared for a render target.");
    for (uint8_t i = 0; i < vkapi::RenderTarget::MaxAttachmentCount; ++i)
    {
        if (desc.attachments.attachArray[i])
        {
            info.desc = desc;

            // get any resources which read/write to this pass
            RenderGraphHandle handle = desc.attachments.attachArray[i];
            for (const auto* edge : readerEdges)
            {
                ResourceNode* node =
                    reinterpret_cast<ResourceNode*>(depGraph.getNode(edge->fromId));
                ASSERT_LOG(node);
                if (node->resourceHandle() == handle)
                {
                    info.readers[i] = node;
                }
            }
            info.writers[i] = rGraph_.getResourceNode(handle);
            if (info.writers[i] == info.readers[i])
            {
                info.writers[i] = nullptr;
            }
        }
    }
    RenderGraphHandle handle {static_cast<uint32_t>(renderPassTargets_.size())};
    renderPassTargets_.emplace_back(info);
    return handle;
}

void RenderPassNode::build()
{
    uint32_t minWidth = std::numeric_limits<uint32_t>::max();
    uint32_t minHeight = std::numeric_limits<uint32_t>::max();
    uint32_t maxWidth = 0;
    uint32_t maxHeight = 0;

    for (auto& renderPassTarget : renderPassTargets_)
    {
        ImportedRenderTarget* importedTarget = nullptr;
        vkapi::RenderPassData& rPassData = renderPassTarget.vkBackend.rPassData;

        for (uint8_t i = 0; i < vkapi::RenderTarget::MaxAttachmentCount; ++i)
        {
            auto& attachment = renderPassTarget.desc.attachments.attachArray[i];

            rPassData.loadClearFlags[i] = vkapi::LoadClearFlags::DontCare;
            rPassData.storeClearFlags[i] = vkapi::StoreClearFlags::Store;

            if (attachment)
            {
                // for depth and stencil clear flags, use the manual settings
                // from the pass setup
                if (i == vkapi::RenderTarget::DepthIndex - 1)
                {
                    rPassData.loadClearFlags[i] = renderPassTarget.desc.dsLoadClearFlags[0];
                    rPassData.storeClearFlags[i] = renderPassTarget.desc.dsStoreClearFlags[0];
                }
                else if (i == vkapi::RenderTarget::StencilIndex - 1)
                {
                    rPassData.loadClearFlags[i] = renderPassTarget.desc.dsLoadClearFlags[1];
                    rPassData.storeClearFlags[i] = renderPassTarget.desc.dsStoreClearFlags[1];
                }
                else
                {
                    // if the pass has no readers then we can clear the store
                    if (renderPassTarget.writers[i] && !renderPassTarget.writers[i]->hasReaders())
                    {
                        rPassData.storeClearFlags[i] = vkapi::StoreClearFlags::DontCare;
                    }
                    // if the pass has no writers then we can clear the load op
                    if (!renderPassTarget.readers[i] || !renderPassTarget.readers[i]->hasWriters())
                    {
                        rPassData.loadClearFlags[i] = vkapi::LoadClearFlags::Clear;
                    }
                }

                // work out the min and max width/height resource size across
                // all targets
                TextureResource* texture =
                    reinterpret_cast<TextureResource*>(rGraph_.getResource(attachment));
                uint32_t textureWidth = texture->descriptor().width;
                uint32_t textureHeight = texture->descriptor().height;
                minWidth = std::min(minWidth, textureWidth);
                minHeight = std::min(minHeight, textureHeight);
                maxWidth = std::max(maxWidth, textureWidth);
                maxHeight = std::max(maxHeight, textureHeight);

                importedTarget =
                    !importedTarget ? texture->asImportedRenderTarget() : importedTarget;
            }
        }

        rPassData.clearCol = renderPassTarget.desc.clearColour;
        rPassData.width = maxWidth;
        rPassData.height = maxHeight;

        // if imported render target, overwrite the render pass data with the
        // imported parameters.
        if (importedTarget)
        {
            rPassData.clearCol = importedTarget->desc.clearColour;
            rPassData.width = importedTarget->desc.width;
            rPassData.height = importedTarget->desc.height;
            rPassData.finalLayouts = importedTarget->desc.finalLayouts;
            renderPassTarget.desc.vkBackend.rtHandle = importedTarget->rtHandle;
            renderPassTarget.imported = true;

            for (size_t i = 0; i < vkapi::RenderTarget::MaxAttachmentCount; ++i)
            {
                if (rPassData.finalLayouts[i] != vk::ImageLayout::eUndefined)
                {
                    rPassData.loadClearFlags[i] = importedTarget->desc.loadClearFlags[i];
                    rPassData.storeClearFlags[i] = importedTarget->desc.storeClearFlags[i];
                }
                else
                {
                    rPassData.loadClearFlags[i] = vkapi::LoadClearFlags::DontCare;
                    rPassData.storeClearFlags[i] = vkapi::StoreClearFlags::DontCare;
                }
            }
        }
    }
}

void RenderPassNode::execute(vkapi::VkDriver& driver, const RenderGraphResource& graphResource)
{
    for (auto& renderPassTarget : renderPassTargets_)
    {
        renderPassTarget.bake(rGraph_);
    }

    rgPass_->execute(driver, graphResource);

    // and delete the targets TODO
}

RenderPassInfo::VkBackend
RenderPassNode::getRenderTargetBackendInfo(const RenderGraphHandle& handle)
{
    RenderPassInfo info = renderPassTargets_[handle.getKey()];
    return info.vkBackend;
}

const RenderPassInfo& RenderPassNode::getRenderTargetInfo(const RenderGraphHandle& handle)
{
    ASSERT_FATAL(
        handle.getKey() < renderPassTargets_.size(),
        "Error whilst getting render target info - key out of limits (key: %d "
        "> size: %d)",
        handle.getKey(),
        renderPassTargets_.size());
    return renderPassTargets_[handle.getKey()];
}

// ====================== PresentPass Node ====================================

PresentPassNode::PresentPassNode(RenderGraph& rGraph) : PassNodeBase(rGraph, "present") {}

PresentPassNode::~PresentPassNode() {}

void PresentPassNode::build() {}

void PresentPassNode::execute(vkapi::VkDriver& driver, const RenderGraphResource& graphResource) {}

} // namespace rg
} // namespace yave
