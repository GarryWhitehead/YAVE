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

#pragma once

#include "utility/bitset_enum.h"
#include "utility/compiler.h"
#include "utility/cstring.h"
#include "vulkan-api/buffer.h"
#include "vulkan-api/common.h"
#include "vulkan-api/renderpass.h"
#include "vulkan-api/texture.h"

#include <cstddef>
#include <cstdint>

// vulkan forward declerations
namespace vkapi
{
class ImageView;
} // namespace vkapi

namespace yave
{
namespace rg
{

// forward declerations
class RenderGraph;
class DependencyGraph;
class RenderPassNode;
class ResourceNode;
class RenderGraphPassBase;
class ResourceEdge;
class PassNodeBase;
class ImportedRenderTarget;

class ResourceBase
{
public:
    virtual ~ResourceBase() = default;
    ResourceBase(util::CString name) : name_(name), parent_(this) {}
    /**
     * @brief constructor for creating a sub-resource (has a parent resource)
     */
    ResourceBase(util::CString name, ResourceBase* parent)
        : name_(name), parent_(parent)
    {
    }

    void registerPass(PassNodeBase* node);

    static bool connectWriter(
        DependencyGraph& graph,
        PassNodeBase* passNode,
        ResourceNode* resourceNode,
        vk::ImageUsageFlags usage);

    static bool connectReader(
        DependencyGraph& graph,
        PassNodeBase* passNode,
        ResourceNode* resourceNode,
        vk::ImageUsageFlags usage);

    virtual void updateResourceUsage(
        DependencyGraph& graph,
        std::vector<std::unique_ptr<ResourceEdge>>& edges,
        ResourceEdge* writer) noexcept
    {
    }

    /**
     * @brief Check if this resource is classed as a sub-resource
     * A sub-resource is a resource that is a child (has a parent)
     */
    bool isSubResource() const;

    virtual bool isImported() const = 0;

    virtual ImportedRenderTarget* asImportedRenderTarget() noexcept
    {
        return nullptr;
    }

    virtual void bake(vkapi::VkDriver& driver) = 0;

    virtual void destroy(vkapi::VkDriver& driver) = 0;

    // =================== getters ==========================

    size_t readCount() const noexcept { return readCount_; }
    ResourceBase* getParent() noexcept { return parent_; }
    PassNodeBase* getFirstPassNode() noexcept { return firstPassNode_; }
    PassNodeBase* getLastPassNode() noexcept { return lastPassNode_; }
    util::CString& getName() noexcept { return name_; }

private:
    util::CString name_; // for degugging

    // ==== set by the compiler =====
    // the number of passes this resource is being used as a input
    size_t readCount_ = 0;

    PassNodeBase* firstPassNode_ = nullptr;
    PassNodeBase* lastPassNode_ = nullptr;

    std::vector<RenderGraphPassBase*> writers_;

    ResourceBase* parent_;
};

// All the information needed to build a vulkan texture
class TextureResource : public ResourceBase
{
public:
    struct Descriptor
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t depth = 1;
        uint8_t mipLevels = 1;
        uint8_t samples = 1;
        vk::Format format = vk::Format::eUndefined;
    };

    TextureResource(const util::CString& name, const Descriptor& desc);

    void updateResourceUsage(
        DependencyGraph& graph,
        std::vector<std::unique_ptr<ResourceEdge>>& edges,
        ResourceEdge* writer) noexcept override;

    void bake(vkapi::VkDriver& driver) override;

    void destroy(vkapi::VkDriver& driver) override;

    bool isDepthFormat();
    bool isColourFormat();
    bool isStencilFormat();

    bool isImported() const override { return false; }

    const Descriptor& descriptor() const { return desc_; }
    vkapi::TextureHandle& handle() { return handle_; }

    friend struct RenderPassInfo;
    friend class RenderPassNode;
    
protected:
    // the image information which will be used to create the image view
    Descriptor desc_;

    // this is resolved only upon calling render graph compile()
    vk::ImageUsageFlags imageUsage_;

    // only valid after call to "bake". 
    // Note: this will be invalid if resource is imported.
    vkapi::TextureHandle handle_;
};

// used for imported texture targets
class ImportedResource : public TextureResource
{
public:
    ImportedResource(
        const util::CString& name,
        const TextureResource::Descriptor& desc,
        vk::ImageUsageFlags imageUsage_,
        vkapi::TextureHandle handle_);

    bool isImported() const override { return true; }

    void bake(vkapi::VkDriver& driver) override;

    void destroy(vkapi::VkDriver& driver) override;
};

class ImportedRenderTarget : public ImportedResource
{
public:
    struct Descriptor
    {
        vkapi::LoadClearFlags
            loadClearFlags[vkapi::RenderTarget::MaxAttachmentCount] = {
                vkapi::LoadClearFlags::DontCare};
        vkapi::StoreClearFlags
            storeClearFlags[vkapi::RenderTarget::MaxAttachmentCount] = {};
          
        std::array<vk::ImageLayout, vkapi::RenderTarget::MaxAttachmentCount> finalLayouts =
            {};

        vk::ImageUsageFlags usage;
        util::Colour4 clearColour {0.0f, 0.0f, 0.0f, 1.0f};

        uint32_t width;
        uint32_t height;
        uint8_t samples = 1;  
    };

    ImportedRenderTarget(
        const util::CString& name,
        const vkapi::RenderTargetHandle& handle,
        const TextureResource::Descriptor& resDesc,
        const Descriptor& importedDesc);
    ~ImportedRenderTarget();

    ImportedRenderTarget* asImportedRenderTarget() noexcept override
    {
        return this;
    }

    // handle to the backend render target which will be imported into the graph
    vkapi::RenderTargetHandle rtHandle;
    Descriptor desc;
};

} // namespace rg
} // namespace yave
