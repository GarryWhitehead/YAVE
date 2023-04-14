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

#include "buffer.h"
#include "commands.h"
#include "common.h"
#include "context.h"
#include "renderpass.h"
#include "pipeline_cache.h"
#include "garbage_collector.h"
#include "utility/compiler.h"
#include "utility/handle.h"

#include <memory>
#include <unordered_set>

namespace vkapi
{
// forward declerations
class Texture;
class Buffer;
class PipelineCache;
class ResourceCache;
class FramebufferCache;
class SamplerCache;
class FramebufferCache;
class ProgramManager;
class ShaderProgramBundle;
class Swapchain;

using VertexBufferHandle = util::Handle<VertexBuffer>;
using IndexBufferHandle = util::Handle<IndexBuffer>;

class VkDriver
{
public:
    VkDriver();
    ~VkDriver();

    bool createInstance(const char** instanceExt, uint32_t count);

    /// initialises the vulkan driver - includes creating the abstract device,
    /// physical device, queues, etc.
    bool init(const vk::SurfaceKHR surface);

    /// Make sure you call this before closing down the engine!
    void shutdown();

    //  ======= Functions for creating buffers and adding resource data to the
    //  backend ======

    /**
     * @brief This is for adding persistant uniform buffers to the backend.
     * These will remain in the backend until the user calls the appropiate
     * destroy function. This function also creates and updates a descriptor set
     * which will be associated with this buffer..
     * @param id This is a string id used to hash and retrieve this buffer
     * @param size The size of the buffer in bytes
     * @param usage Vulkan usage flags depict what this buffer will be used for
     */
    BufferHandle addUbo(const size_t size, VkBufferUsageFlags usage);

    /**
     * @brief Adds a vertex buffer to the vulkan back end. This function also
     * generates the vertex attribute bindings in preperation to using with the
     * relevant pipeline
     */
    VertexBufferHandle addVertexBuffer(const size_t size, void* data);

    void
    mapVertexBuffer(const VertexBufferHandle& handle, size_t count, void* data); 

    /**
     * @brief Similiar to the **addVertexBuffer** function, adds a index buffer
     * to the vulkan backend. Note: it is presumed to be of the type uint32_t.
     */
    IndexBufferHandle addIndexBuffer(const size_t size, void* data);

    void
    mapIndexBuffer(const IndexBufferHandle& handle, size_t count, void* data);

    VertexBuffer* getVertexBuffer(const VertexBufferHandle& vbHandle) noexcept;

    IndexBuffer* getIndexBuffer(const IndexBufferHandle& ibHandle) noexcept;

    RenderTargetHandle createRenderTarget(
        const ::util::CString& name,
        uint32_t width,
        uint32_t height,
        uint8_t samples,
        const util::Colour4& clearCol,
        const std::array<
            RenderTarget::AttachmentInfo,
            vkapi::RenderTarget::MaxColourAttachCount>& colours,
        const RenderTarget::AttachmentInfo& depth,
        const RenderTarget::AttachmentInfo& stencil);

    vk::Format getSupportedDepthFormat() const;

    // =============== delete buffer =======================================

    void deleteVertexBuffer(const VertexBufferHandle& handle);

    void deleteIndexBuffer(const IndexBufferHandle& handle);

    // ======== begin/end frame functions =================================

    bool beginFrame(Swapchain& swapchain);

    void endFrame(Swapchain& swapchain);

    void beginRenderpass(
        vk::CommandBuffer cmds, const RenderPassData& data, const RenderTargetHandle& rtHandle);

    void endRenderpass(vk::CommandBuffer& cmdBuffer);

    Commands& getCommands() noexcept;

    // ============= retrieve and delete  resources ============================

    void mapTexture(
        const TextureHandle& handle,
        void* data,
        uint32_t dataSize,
        size_t* offsets);

    TextureHandle createTexture2d(
        vk::Format format,
        uint32_t width,
        uint32_t height,
        uint8_t mipLevels,
        uint8_t faceCount,
        uint8_t arrayCount,
        vk::ImageUsageFlags usageFlags);

    TextureHandle createTexture2d(
        vk::Format format, uint32_t width, uint32_t height, vk::Image image);

    void destroyTexture2D(TextureHandle& handle);

    void destroyBuffer(BufferHandle& handle);

    void draw(
        vk::CommandBuffer cmdBuffer,
        ShaderProgramBundle& programBundle,
        vk::Buffer vertexBuffer = VK_NULL_HANDLE,
        vk::Buffer indexBuffer = VK_NULL_HANDLE,
        vk::VertexInputAttributeDescription* vertexAttr = nullptr,
        vk::VertexInputBindingDescription* vertexBinding = nullptr,
        const std::vector<uint32_t>& dynamicOffsets = {});

    uint32_t getCurrentPresentIndex() const noexcept
    {
        ASSERT_LOG(imageIndex_ != UINT32_MAX);
        return imageIndex_;
    }

    void collectGarbage() noexcept;

    // =============== getters =============================================

    VkContext& context() { return *context_; }
    VmaAllocator& vmaAlloc() { return vmaAlloc_; }
    const vk::Semaphore& imageSignal() const { return imageReadySignal_; }
    const StagingPool& stagingPool() const { return *stagingPool_.get(); }
    ProgramManager& progManager() { return *programManager_; }
    PipelineCache& pipelineCache() { return *pipelineCache_; }
    SamplerCache& getSamplerCache() { return *samplerCache_; }
    uint64_t getCurrentFrame() const noexcept { return currentFrame_; }

    using VertexBufferMap = std::vector<VertexBuffer*>;
    using IndexBufferMap = std::vector<IndexBuffer*>;

private:
    /// the current device context
    std::unique_ptr<VkContext> context_;

    /// The current present KHR frame image index
    uint32_t imageIndex_;

    /// external mem allocator
    VmaAllocator vmaAlloc_;

    /// staging pool used for managing CPU stages
    std::unique_ptr<StagingPool> stagingPool_;

    std::unique_ptr<ProgramManager> programManager_;

    std::vector<RenderTarget> renderTargets_;

    // vertex/index buffers
    VertexBufferMap vertBuffers_;
    IndexBufferMap indexBuffers_;

    // caches
    std::unique_ptr<ResourceCache> resourceCache_;
    std::unique_ptr<PipelineCache> pipelineCache_;
    std::unique_ptr<FramebufferCache> framebufferCache_;
    std::unique_ptr<SamplerCache> samplerCache_;
    
    std::unique_ptr<Commands> commands_;
    
    GarbageCollector gc;

    /// used for ensuring that the image has completed
    vk::Semaphore imageReadySignal_;

    /// The frame number as designated by the number of times
    /// a presentation quque flush has been carried out.
    uint64_t currentFrame_;
};


} // namespace vkapi
