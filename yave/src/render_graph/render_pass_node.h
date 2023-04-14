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
#include "render_graph_handle.h"
#include "render_graph_pass.h"
#include "utility/colour.h"
#include "utility/cstring.h"
#include "vulkan-api/renderpass.h"

#include <vector>

namespace yave
{
namespace rg
{
// forward declerations
class RenderGraphResource;

/**
 * @brief All theinformation required to create a concrete vulkan renderpass
 */
struct RenderPassInfo
{
    RenderPassInfo() = default;
    util::CString name;
    ResourceNode* readers[vkapi::RenderTarget::MaxAttachmentCount] = {};
    ResourceNode* writers[vkapi::RenderTarget::MaxAttachmentCount] = {};
    PassDescriptor desc = {};
    bool imported = false;

    struct VkBackend
    {
        vkapi::RenderPassData rPassData;
    } vkBackend;

    /**
     * @brief Create a concrete vulkan renderpass object.
     * @param rGraph A initilaised render graph.
     */
    void bake(const RenderGraph& rGraph);
};

class PassNodeBase : public Node
{
public:
    PassNodeBase(RenderGraph& rGraph, const util::CString& name);
    virtual ~PassNodeBase();

    virtual void build() = 0;
    virtual void execute(
        vkapi::VkDriver& driver, const RenderGraphResource& graphResource) = 0;

    void addToBakeList(ResourceBase* res);

    void addToDestroyList(ResourceBase* res);

    void bakeResourceList(vkapi::VkDriver& driver);

    void destroyResourceList(vkapi::VkDriver& driver);

    void addResource(const RenderGraphHandle& handle);

protected:
    RenderGraph& rGraph_;

    std::vector<ResourceBase*> resourcesToBake_;
    std::vector<ResourceBase*> resourcesToDestroy_;
    std::vector<RenderGraphHandle> resourceHandles_;
};

class RenderPassNode : public PassNodeBase
{
public:
    RenderPassNode(
        RenderGraph& rGraph,
        RenderGraphPassBase* rgPass,
        const util::CString& name);

    RenderGraphHandle
    createRenderTarget(const util::CString& name, const PassDescriptor& desc);

    RenderPassInfo::VkBackend
    getRenderTargetBackendInfo(const RenderGraphHandle& handle);

    void build() override;

    void execute(
        vkapi::VkDriver& driver,
        const RenderGraphResource& graphResource) override;

    const RenderPassInfo& getRenderTargetInfo(const RenderGraphHandle& handle);

private:
    RenderGraphPassBase* rgPass_;

    std::vector<RenderPassInfo> renderPassTargets_;
};

class PresentPassNode : public PassNodeBase
{
public:
    PresentPassNode(RenderGraph& rGraph);
    ~PresentPassNode();

    void build() override;

    void execute(
        vkapi::VkDriver& driver,
        const RenderGraphResource& graphResource) override;

private:
};

} // namespace rg
} // namespace yave
