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

#include "driver.h"

#include "../backend/enums.h"
#include "framebuffer_cache.h"
#include "image.h"
#include "pipeline.h"
#include "pipeline_cache.h"
#include "program_manager.h"
#include "renderpass.h"
#include "resource_cache.h"
#include "sampler_cache.h"
#include "swapchain.h"
#include "texture.h"
#include "utility.h"
#include "utility/assertion.h"
#include "utility/cstring.h"
#include "utility/murmurhash.h"

#include <spdlog/spdlog.h>

namespace vkapi
{

// =================== driver ==============================

VkDriver::VkDriver()
    : context_(std::make_unique<VkContext>()),
      imageIndex_(UINT32_MAX),
      programManager_(std::make_unique<ProgramManager>(*this)),
      resourceCache_(std::make_unique<ResourceCache>(*context_, *this)),
      pipelineCache_(std::make_unique<PipelineCache>(*context_, *this)),
      framebufferCache_(std::make_unique<FramebufferCache>(*context_, *this)),
      samplerCache_(std::make_unique<SamplerCache>(*this)),
      currentFrame_(0)
{
}

VkDriver::~VkDriver() {}

bool VkDriver::createInstance(const char** instanceExt, uint32_t count)
{
    // create a new vulkan instance
    if (!context_->createInstance(instanceExt, count))
    {
        return false;
    }
    return true;
}

bool VkDriver::init(const vk::SurfaceKHR surface)
{
    // prepare the vulkan backend
    if (!context_->prepareDevice(surface))
    {
        return false;
    }

    pipelineCache_->init();

    // set up the memory allocator
    VmaAllocatorCreateInfo createInfo = {};
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    createInfo.physicalDevice = context_->physical();
    createInfo.device = context_->device();
    createInfo.instance = context_->instance();
    VkResult result = vmaCreateAllocator(&createInfo, &vmaAlloc_);
    ASSERT_LOG(result == VK_SUCCESS);

    // create the staging pool
    stagingPool_ = std::make_unique<StagingPool>(vmaAlloc_);

    // command buffers for graphics and presentation - we make the assumption
    // that both queues are the same which is the case on all common devices.
    commands_ = std::make_unique<Commands>(*this, context().graphicsQueue());

    // create a semaphore for signalling that a image is ready for presentation
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_CHECK_RESULT(
        context().device().createSemaphore(&semaphoreCreateInfo, nullptr, &imageReadySignal_));

    return true;
}

void VkDriver::shutdown()
{
    context_->device().destroy(imageReadySignal_, nullptr);
    vmaDestroyAllocator(vmaAlloc_);
}


RenderTargetHandle VkDriver::createRenderTarget(
    const ::util::CString& name,
    uint32_t width,
    uint32_t height,
    uint8_t samples,
    const util::Colour4& clearCol,
    const std::array<RenderTarget::AttachmentInfo, RenderTarget::MaxColourAttachCount>& colours,
    const RenderTarget::AttachmentInfo& depth,
    const RenderTarget::AttachmentInfo& stencil)
{
    RenderTarget rt;
    rt.depth = depth;
    rt.stencil = stencil;
    rt.clearCol = clearCol;
    memcpy(
        &rt.colours,
        colours.data(),
        sizeof(RenderTarget::AttachmentInfo) * RenderTarget::MaxColourAttachCount);

    RenderTargetHandle handle {static_cast<uint32_t>(renderTargets_.size())};
    renderTargets_.emplace_back(rt);
    return handle;
}

// =========== functions for buffer/texture creation ================


VertexBufferHandle VkDriver::addVertexBuffer(const size_t size, void* data)
{
    ASSERT_FATAL(data, "Data is nullptr when trying to add vertex buffer to vk backend.");
    VertexBuffer* buffer = new VertexBuffer();
    buffer->create(*this, *context_, vmaAlloc_, *stagingPool_, data, size);
    VertexBufferHandle handle {static_cast<uint32_t>(vertBuffers_.size())};
    vertBuffers_.emplace_back(std::move(buffer));
    return handle;
}

void VkDriver::mapVertexBuffer(const VertexBufferHandle& handle, size_t count, void* data)
{
    ASSERT_FATAL(data, "Can not map vertex buffer when data pointer is NULL.");

    VertexBuffer* buffer = getVertexBuffer(handle);
    ASSERT_LOG(buffer);

    if (count > buffer->getSize())
    {
        deleteVertexBuffer(handle);
        addVertexBuffer(count, data);
        return;
    }
    buffer->mapAndCopyToGpu(*this, *stagingPool_, count, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data);
}

IndexBufferHandle VkDriver::addIndexBuffer(const size_t size, void* data)
{
    ASSERT_FATAL(data, "Data is nullptr when trying to add index buffer to vk backend.");
    IndexBuffer* buffer = new IndexBuffer();
    buffer->create(*this, *context_, vmaAlloc_, *stagingPool_, data, size);
    IndexBufferHandle handle {static_cast<uint32_t>(indexBuffers_.size())};
    indexBuffers_.emplace_back(std::move(buffer));
    return handle;
}

void VkDriver::mapIndexBuffer(const IndexBufferHandle& handle, size_t count, void* data)
{
    ASSERT_FATAL(data, "Can not map vertex buffer when data pointer is NULL.");

    IndexBuffer* buffer = getIndexBuffer(handle);
    ASSERT_LOG(buffer);

    if (count > buffer->getSize())
    {
        deleteIndexBuffer(handle);
        addIndexBuffer(count, data);
        return;
    }
    buffer->mapAndCopyToGpu(*this, *stagingPool_, count, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, data);
}

VertexBuffer* VkDriver::getVertexBuffer(const VertexBufferHandle& vbHandle) noexcept
{
    ASSERT_FATAL(
        vbHandle.getKey() < vertBuffers_.size(),
        "Invalid vertex buffer handle: %d",
        vbHandle.getKey());
    return vertBuffers_[vbHandle.getKey()];
}

IndexBuffer* VkDriver::getIndexBuffer(const IndexBufferHandle& ibHandle) noexcept
{
    ASSERT_FATAL(
        ibHandle.getKey() < indexBuffers_.size(),
        "Invalid index buffer handle: %d",
        ibHandle.getKey());
    return indexBuffers_[ibHandle.getKey()];
}

void VkDriver::deleteVertexBuffer(const VertexBufferHandle& handle)
{
    ASSERT_FATAL(
        handle.getKey() < vertBuffers_.size(), "Invalid vertex buffer handle: %d", handle.getKey());
    VertexBuffer* buffer = vertBuffers_[handle.getKey()];
    // We get the vulkan buffer handle so its safe to delete the associated
    // VertexBuffer object.
    gc.add([buffer, this]() {
        buffer->destroy(vmaAlloc_);
        delete buffer;
    });
    vertBuffers_.erase(vertBuffers_.begin() + handle.getKey());
}

void VkDriver::deleteIndexBuffer(const IndexBufferHandle& handle)
{
    ASSERT_FATAL(
        handle.getKey() < indexBuffers_.size(), "Invalid index buffer handle: %d", handle.getKey());
    IndexBuffer* buffer = indexBuffers_[handle.getKey()];
    gc.add([buffer, this]() {
        buffer->destroy(vmaAlloc_);
        delete buffer;
    });
    indexBuffers_.erase(indexBuffers_.begin() + handle.getKey());
}

// ============ begin/end frame functions ======================

bool VkDriver::beginFrame(Swapchain& swapchain)
{
    // get the next image index which will be the framebuffer we draw too
    vk::Result result = context_->device().acquireNextImageKHR(
        swapchain.get(), std::numeric_limits<uint64_t>::max(), imageReadySignal_, {}, &imageIndex_);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        return false;
    }

    ASSERT_LOG(result == vk::Result::eSuccess);
    return true;
}

void VkDriver::endFrame(Swapchain& swapchain)
{
    commands_->setExternalWaitSignal(&imageReadySignal_);

    // submit the present cmd buffer and send to the queue
    commands_->flush();

    vk::Semaphore* renderCompleteSignal = commands_->getFinishedSignal();

    vk::PresentInfoKHR presentInfo {
        1, renderCompleteSignal, 1, &swapchain.get(), &imageIndex_, nullptr};
    VK_CHECK_RESULT(context().presentQueue().presentKHR(&presentInfo));

    SPDLOG_INFO(
        "KHR Presentation (image index {}) - render wait signal: {:p}",
        imageIndex_,
        fmt::ptr((void*)*renderCompleteSignal));

    currentFrame_++;

    // destroy any resources which have reached there use by date
    collectGarbage();
}

void VkDriver::beginRenderpass(
    vk::CommandBuffer cmds, const RenderPassData& data, const RenderTargetHandle& rtHandle)
{
    ASSERT_LOG(rtHandle);
    RenderTarget& renderTarget = renderTargets_[rtHandle.getKey()];
    RenderTarget::AttachmentInfo depth = renderTarget.depth;

    // find a renderpass from the cache or create a new one.
    FramebufferCache::RPassKey rpassKey;
    if (depth.handle)
    {
        auto depthTexture = depth.handle.getResource();
        rpassKey.depth = depthTexture->context().format;
    }
    rpassKey.samples = renderTarget.samples;

    for (int i = 0; i < RenderTarget::MaxColourAttachCount; ++i)
    {
        RenderTarget::AttachmentInfo colour = renderTarget.colours[i];
        if (colour.handle)
        {
            auto tex = colour.handle.getResource();
            rpassKey.colourFormats[i] = tex->context().format;
            ASSERT_LOG(data.finalLayouts[i] != vk::ImageLayout::eUndefined);
            rpassKey.finalLayout[i] = data.finalLayouts[i];
            rpassKey.loadOp[i] = data.loadClearFlags[i];
            rpassKey.storeOp[i] = data.storeClearFlags[i];
        }
        else
        {
            rpassKey.colourFormats[i] = vk::Format::eUndefined;
        }
    }
    rpassKey.dsLoadOp[0] = data.loadClearFlags[RenderTarget::DepthIndex - 1];
    rpassKey.dsStoreOp[0] = data.storeClearFlags[RenderTarget::DepthIndex - 1];
    rpassKey.dsLoadOp[1] = data.loadClearFlags[RenderTarget::StencilIndex - 1];
    rpassKey.dsStoreOp[1] = data.storeClearFlags[RenderTarget::StencilIndex - 1];

    RenderPass* rpass = framebufferCache_->findOrCreateRenderPass(rpassKey);
    rpass->lastUsedFrameStamp_ = currentFrame_;

    // find a framebuffer from the cache or create a new one.
    FramebufferCache::FboKey fboKey;
    fboKey.renderpass = rpass->get();
    fboKey.width = data.width;
    fboKey.height = data.height;
    fboKey.samples = rpassKey.samples;

    int count = 0;
    for (int idx = 0; idx < RenderTarget::MaxColourAttachCount; ++idx)
    {
        RenderTarget::AttachmentInfo colour = renderTarget.colours[idx];
        if (colour.handle)
        {
            auto tex = colour.handle.getResource();
            ASSERT_LOG(colour.handle);
            fboKey.views[idx] = tex->getImageView()->get();
            ASSERT_FATAL(fboKey.views[idx], "ImageView for attachment %d is invalid.", idx);
            ++count;
        }
    }
    if (renderTarget.depth.handle)
    {
        auto tex = renderTarget.depth.handle.getResource();
        fboKey.views[count++] = tex->getImageView()->get();
    }

    FrameBuffer* fbo = framebufferCache_->findOrCreateFrameBuffer(fboKey, count);
    fbo->lastUsedFrameStamp_ = currentFrame_;

    // sort out the clear values for the pass
    std::vector<vk::ClearValue> clearValues;

    // setup the clear values for this pass - need one for each attachment
    auto& attachments = rpass->getAttachments();
    clearValues.resize(attachments.size());

    for (size_t i = 0; i < attachments.size(); ++i)
    {
        if (isDepth(attachments[i].format) || isStencil(attachments[i].format))
        {
            clearValues[attachments.size() - 1].depthStencil = vk::ClearDepthStencilValue {1.0f, 0};
        }
        else
        {
            clearValues[i].color.float32[0] = renderTarget.clearCol.r();
            clearValues[i].color.float32[1] = renderTarget.clearCol.g();
            clearValues[i].color.float32[2] = renderTarget.clearCol.b();
            clearValues[i].color.float32[3] = renderTarget.clearCol.a();
        }
    }

    // extents of the frame buffer
    vk::Rect2D extents {{0, 0}, {fbo->getWidth(), fbo->getHeight()}};

    vk::RenderPassBeginInfo beginInfo {
        rpass->get(),
        fbo->get(),
        extents,
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()};

    vk::SubpassContents contents = vk::SubpassContents::eInline;
    cmds.beginRenderPass(beginInfo, contents);

    // use custom defined viewing area - at the moment set to the framebuffer
    // size
    vk::Viewport viewport {
        0.0f,
        0.0f,
        static_cast<float>(fbo->getWidth()),
        static_cast<float>(fbo->getHeight()),
        0.0f,
        1.0f};
    pipelineCache_->bindViewport(cmds, viewport);

    vk::Rect2D scissor {
        {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
        {static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)}};
    pipelineCache_->bindScissor(cmds, scissor);

    // bind the renderpass to the pipeline
    pipelineCache_->bindRenderPass(rpass->get());
    pipelineCache_->bindColourAttachCount(rpass->colAttachCount());
}

void VkDriver::endRenderpass(vk::CommandBuffer& cmdBuffer) { cmdBuffer.endRenderPass(); }

Commands& VkDriver::getCommands() noexcept { return *commands_; }


void VkDriver::draw(
    vk::CommandBuffer cmdBuffer,
    ShaderProgramBundle& programBundle,
    vk::Buffer vertexBuffer,
    vk::Buffer indexBuffer,
    vk::VertexInputAttributeDescription* vertexAttr,
    vk::VertexInputBindingDescription* vertexBinding,
    const std::vector<uint32_t>& dynamicOffsets)
{
    // TODO: the pipelinelayout should be cached within the pipeline cache
    // as at present its created in the shader program bundle which means
    // that we are creating layouts which may already exist within other bundles
    // and its tricky destroying the layout along with the corresponding
    // descriptor sets.
    PipelineLayout& plineLayout = programBundle.getPipelineLayout();

    // Bind the texture samplers for each shader stage
    vkapi::PipelineCache::DescriptorImage samplers[vkapi::PipelineCache::MaxSamplerBindCount];
    for (int idx = 0; idx < vkapi::PipelineCache::MaxSamplerBindCount; ++idx)
    {
        const vkapi::TextureHandle& handle = programBundle.textureHandles_[idx];
        if (handle)
        {
            ASSERT_FATAL(
                programBundle.samplers_[idx], "Image sampler has not been set for this material.");

            const auto& tex = handle.getResource();
            vkapi::PipelineCache::DescriptorImage& image = samplers[idx];
            image.imageSampler = programBundle.samplers_[idx];
            image.imageView = tex->getImageView()->get();
            image.imageLayout = tex->getImageLayout();
        }
    }
    pipelineCache_->bindSampler(samplers);

    // Bind all the buffers associated with this pipeline
    for (const auto& info : programBundle.descBindInfo_)
    {
        if (info.type == vk::DescriptorType::eUniformBuffer)
        {
            pipelineCache_->bindUbo(info.binding, info.buffer, info.size);
        }
        else if (info.type == vk::DescriptorType::eUniformBufferDynamic)
        {
            pipelineCache_->bindUboDynamic(info.binding, info.buffer, info.size);
        }
        else if (info.type == vk::DescriptorType::eStorageBuffer)
        {
            pipelineCache_->bindSsbo(info.binding, info.buffer, info.size);
        }
    }
    plineLayout.build(context());
    pipelineCache_->bindDescriptors(cmdBuffer, plineLayout, dynamicOffsets);

    // Bind the pipeline.
    pipelineCache_->bindShaderModules(programBundle);

    // Bind the rasterisation and depth/stencil states
    auto& srcRasterState = programBundle.rasterState_;
    auto& srcDsState = programBundle.dsState_;
    auto& srcBlendState = programBundle.blendState_;

    pipelineCache_->bindCullMode(srcRasterState.cullMode);
    pipelineCache_->bindFrontFace(srcRasterState.frontFace);
    pipelineCache_->bindPolygonMode(srcRasterState.polygonMode);
    pipelineCache_->bindDepthTestEnable(srcDsState.testEnable);
    pipelineCache_->bindDepthWriteEnable(srcDsState.writeEnable);

    // TODO: Need to support differences in front/back stencil
    PipelineCache::DepthStencilBlock dsBlock;
    dsBlock.compareOp = srcDsState.frontStencil.compareOp;
    dsBlock.compareMask = srcDsState.frontStencil.compareMask;
    dsBlock.depthFailOp = srcDsState.frontStencil.depthFailOp;
    dsBlock.passOp = srcDsState.frontStencil.passOp;
    dsBlock.reference = srcDsState.frontStencil.reference;
    dsBlock.stencilFailOp = srcDsState.frontStencil.stencilFailOp;
    dsBlock.stencilTestEnable = srcDsState.stencilTestEnable;
    pipelineCache_->bindDepthStencilBlock(dsBlock);

    // blend factors
    PipelineCache::BlendFactorBlock blendState;
    blendState.blendEnable = srcBlendState.blendEnable;
    blendState.srcColorBlendFactor = srcBlendState.srcColor;
    blendState.dstColorBlendFactor = srcBlendState.dstColor;
    blendState.colorBlendOp = srcBlendState.colour;
    blendState.srcAlphaBlendFactor = srcBlendState.srcAlpha;
    blendState.dstAlphaBlendFactor = srcBlendState.dstAlpha;
    blendState.alphaBlendOp = srcBlendState.alpha;
    pipelineCache_->bindBlendFactorBlock(blendState);

    // Bind primitive info
    pipelineCache_->bindPrimRestart(programBundle.renderPrim_.primitiveRestart);
    pipelineCache_->bindTopology(programBundle.renderPrim_.topology);

    // if the width and height are zero then ignore setting the scissor and/or
    // viewport and go with the extents set aupon initiation of the renderpass
    if (programBundle.scissor_.extent.width != 0 && programBundle.scissor_.extent.height != 0)
    {
        pipelineCache_->bindScissor(cmdBuffer, programBundle.scissor_);
    }
    if (programBundle.viewport_.width != 0 && programBundle.viewport_.height != 0)
    {
        pipelineCache_->bindViewport(cmdBuffer, programBundle.viewport_);
    }

    // Bind the vertex attributes and input info. This has been computed
    // in advance so just a matter of passing the values to the pipeline cache.
    if (vertexAttr && vertexBinding)
    {
        pipelineCache_->bindVertexInput(vertexAttr, vertexBinding);
    }

    pipelineCache_->bindPipeline(cmdBuffer, plineLayout);

    // Bind the push block if we have one. Note: The binding of the pushblock
    // has to be done after the binding of the pipeline.
    for (int i = 0; i < 2; i++)
    {
        if (programBundle.pushBlock_[i])
        {
            ASSERT_FATAL(
                programBundle.pushBlock_[i]->data, "No data has been set for this pushblock.");
            plineLayout.bindPushBlock(cmdBuffer, *programBundle.pushBlock_[i]);
        }
    }

    // We only used interleaved vertex data so this will only ever
    // be binding a single buffer and the offset will be zero.
    if (vertexBuffer)
    {
        vk::DeviceSize offset[1] = {0};
        cmdBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offset);
    }
    if (indexBuffer)
    {
        cmdBuffer.bindIndexBuffer(indexBuffer, 0, programBundle.renderPrim_.indexBufferType);
        cmdBuffer.drawIndexed(
            programBundle.renderPrim_.indicesCount, 1, programBundle.renderPrim_.offset, 0, 0);
    }
    else
    {
        ASSERT_FATAL(
            programBundle.renderPrim_.vertexCount > 0,
            "When no index buffer is declared, the vertex count must be "
            "specified.");
        cmdBuffer.draw(programBundle.renderPrim_.vertexCount, 1, 0, 0);
    }
}

vk::Format VkDriver::getSupportedDepthFormat() const
{
    // in order of preference - TODO: allow user to define whether stencil
    // format is required or not
    std::vector<vk::Format> formats = {
        vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD32Sfloat};
    vk::FormatFeatureFlags formatFeature = vk::FormatFeatureFlagBits::eDepthStencilAttachment;

    vk::Format output;
    for (auto format : formats)
    {
        vk::FormatProperties properties = context_->physical().getFormatProperties(format);
        if (formatFeature == (properties.optimalTilingFeatures & formatFeature))
        {
            output = format;
            break;
        }
    }
    return output;
}

BufferHandle VkDriver::addUbo(const size_t size, VkBufferUsageFlags usage)
{
    return resourceCache_->createUbo(size, usage);
}

TextureHandle VkDriver::createTexture2d(
    vk::Format format,
    uint32_t width,
    uint32_t height,
    uint8_t mipLevels,
    uint8_t faceCount,
    uint8_t arrayCount,
    vk::ImageUsageFlags usageFlags)
{
    return resourceCache_->createTexture2d(
        format, width, height, mipLevels, usageFlags, faceCount, arrayCount);
}

TextureHandle
VkDriver::createTexture2d(vk::Format format, uint32_t width, uint32_t height, vk::Image image)
{
    return resourceCache_->createTexture2d(format, width, height, image);
}

void VkDriver::mapTexture(
    const TextureHandle& handle, void* data, uint32_t dataSize, size_t* offsets)
{
    Texture* tex = const_cast<Texture*>(handle.getResource());
    tex->map(*this, data, dataSize, offsets);
}

void VkDriver::destroyTexture2D(TextureHandle& handle)
{
    resourceCache_->deleteTexture(handle, gc);
}

void VkDriver::destroyBuffer(BufferHandle& handle) { resourceCache_->deleteUbo(handle, gc); }

void VkDriver::collectGarbage() noexcept
{
    gc.collectGarbage();
    framebufferCache_->cleanCache(currentFrame_);
    pipelineCache_->cleanCache(currentFrame_);
    resourceCache_->garbageCollection();
    stagingPool_->garbageCollection(currentFrame_);
}

} // namespace vkapi
