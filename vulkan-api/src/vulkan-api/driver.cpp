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

VkDriver::~VkDriver() = default;

bool VkDriver::createInstance(const char** instanceExt, uint32_t count)
{
    // create a new vulkan instance
    if (!context_->createInstance(instanceExt, count))
    {
        return false;
    }
    return true;
}

bool VkDriver::init(vk::SurfaceKHR surface)
{
    // prepare the vulkan backend
    if (!context_->prepareDevice(surface))
    {
        return false;
    }

    pipelineCache_->init();

    // set up the memory allocator
    VmaAllocatorCreateInfo createInfo = {};
    createInfo.vulkanApiVersion = VK_API_VERSION_1_2;
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
    bool multiView,
    const util::Colour4& clearCol,
    const std::array<RenderTarget::AttachmentInfo, RenderTarget::MaxColourAttachCount>& colours,
    const RenderTarget::AttachmentInfo& depth,
    const RenderTarget::AttachmentInfo& stencil)
{
    RenderTarget rt;
    rt.depth = depth;
    rt.stencil = stencil;
    rt.clearCol = clearCol;
    rt.multiView = multiView;
    std::copy(colours.begin(), colours.end(), rt.colours.begin());

    RenderTargetHandle handle {static_cast<uint32_t>(renderTargets_.size())};
    renderTargets_.emplace_back(rt);
    return handle;
}

// =========== functions for buffer/texture creation ================


VertexBufferHandle VkDriver::addVertexBuffer(size_t size, void* data)
{
    ASSERT_FATAL(data, "Data is nullptr when trying to add vertex buffer to vk backend.");
    auto* buffer = new VertexBuffer();
    buffer->create(*this, vmaAlloc_, *stagingPool_, data, size);
    VertexBufferHandle handle {static_cast<uint32_t>(vertBuffers_.size())};
    vertBuffers_.emplace_back(buffer);
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
    buffer->mapAndCopyToGpu(*this, count, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data);
}

IndexBufferHandle VkDriver::addIndexBuffer(size_t size, void* data)
{
    ASSERT_FATAL(data, "Data is nullptr when trying to add index buffer to vk backend.");
    auto* buffer = new IndexBuffer();
    buffer->create(*this, vmaAlloc_, *stagingPool_, data, size);
    IndexBufferHandle handle {static_cast<uint32_t>(indexBuffers_.size())};
    indexBuffers_.emplace_back(buffer);
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
    buffer->mapAndCopyToGpu(*this, count, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, data);
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
    // We get the vulkan buffer handle, so it's safe to delete the associated
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

    SPDLOG_DEBUG(
        "KHR Presentation (image index {}) - render wait signal: {:p}",
        imageIndex_,
        fmt::ptr((void*)*renderCompleteSignal));

    // destroy any resources which have reached there use by date
    collectGarbage();

    currentFrame_++;
}

void VkDriver::beginRenderpass(
    vk::CommandBuffer cmds, const RenderPassData& data, const RenderTargetHandle& rtHandle)
{
    ASSERT_FATAL(rtHandle, "Invalid render target handle.");

    RenderTarget& renderTarget = renderTargets_[rtHandle.getKey()];
    RenderTarget::AttachmentInfo depth = renderTarget.depth;

    // find a renderpass from the cache or create a new one.
    FramebufferCache::RPassKey rpassKey {};
    rpassKey.depth = vk::Format::eUndefined;
    if (depth.handle)
    {
        auto depthTexture = depth.handle.getResource();
        rpassKey.depth = depthTexture->context().format;
    }
    rpassKey.samples = renderTarget.samples;
    rpassKey.multiView = renderTarget.multiView;

    for (int i = 0; i < RenderTarget::MaxColourAttachCount; ++i)
    {
        RenderTarget::AttachmentInfo colour = renderTarget.colours[i];
        rpassKey.colourFormats[i] = vk::Format::eUndefined;
        if (colour.handle)
        {
            Texture* tex = colour.handle.getResource();
            rpassKey.colourFormats[i] = tex->context().format;
            ASSERT_LOG(data.finalLayouts[i] != vk::ImageLayout::eUndefined);
            rpassKey.finalLayout[i] = data.finalLayouts[i];
            rpassKey.initialLayout[i] = data.initialLayouts[i];
            rpassKey.loadOp[i] = data.loadClearFlags[i];
            rpassKey.storeOp[i] = data.storeClearFlags[i];
        }
    }
    rpassKey.dsLoadOp[0] = data.loadClearFlags[RenderTarget::DepthIndex - 1];
    rpassKey.dsStoreOp[0] = data.storeClearFlags[RenderTarget::DepthIndex - 1];
    rpassKey.dsLoadOp[1] = data.loadClearFlags[RenderTarget::StencilIndex - 1];
    rpassKey.dsStoreOp[1] = data.storeClearFlags[RenderTarget::StencilIndex - 1];

    RenderPass* rpass = framebufferCache_->findOrCreateRenderPass(rpassKey);

    // find a framebuffer from the cache or create a new one.
    FramebufferCache::FboKey fboKey;
    fboKey.renderpass = rpass->get();
    fboKey.width = data.width;
    fboKey.height = data.height;
    fboKey.samples = rpassKey.samples;
    fboKey.layer = 1;

    int count = 0;
    for (int idx = 0; idx < RenderTarget::MaxColourAttachCount; ++idx)
    {
        RenderTarget::AttachmentInfo colour = renderTarget.colours[idx];
        if (colour.handle)
        {
            vkapi::Texture* tex = colour.handle.getResource();
            fboKey.views[idx] = tex->getImageView(colour.level)->get();
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

    // set up the clear values for this pass - need one for each attachment
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
        const vkapi::TextureHandle& handle = programBundle.imageSamplers_[idx].texHandle;
        vk::Sampler sampler = programBundle.imageSamplers_[idx].sampler;

        if (handle)
        {
            const auto& tex = handle.getResource();
            vkapi::PipelineCache::DescriptorImage& image = samplers[idx];
            image.imageSampler = sampler;
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
    pipelineCache_->bindGraphicsShaderModules(programBundle);

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
    PipelineCache::DepthStencilBlock dsBlock {};
    dsBlock.compareOp = srcDsState.frontStencil.compareOp;
    dsBlock.compareMask = srcDsState.frontStencil.compareMask;
    dsBlock.depthFailOp = srcDsState.frontStencil.depthFailOp;
    dsBlock.passOp = srcDsState.frontStencil.passOp;
    dsBlock.reference = srcDsState.frontStencil.reference;
    dsBlock.stencilFailOp = srcDsState.frontStencil.stencilFailOp;
    dsBlock.stencilTestEnable = srcDsState.stencilTestEnable;
    pipelineCache_->bindDepthStencilBlock(dsBlock);

    // blend factors
    PipelineCache::BlendFactorBlock blendState {};
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
    pipelineCache_->bindTesselationVertCount(programBundle.tesselationVertCount_);

    // if the width and height are zero then ignore setting the scissors and/or
    // viewport and go with the extents set upon initiation of the renderpass
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

    pipelineCache_->bindGraphicsPipeline(cmdBuffer, plineLayout);

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

void VkDriver::dispatchCompute(
    vk::CommandBuffer& cmd,
    ShaderProgramBundle* bundle,
    uint32_t xWorkCount,
    uint32_t yWorkCount,
    uint32_t zWorkCount)
{
    PipelineLayout& plineLayout = bundle->getPipelineLayout();

    // image storage
    vkapi::PipelineCache::DescriptorImage
        storageImages[vkapi::PipelineCache::MaxStorageImageBindCount];
    for (int idx = 0; idx < vkapi::PipelineCache::MaxStorageImageBindCount; ++idx)
    {
        const vkapi::TextureHandle& handle = bundle->storageImages_[idx];
        if (handle)
        {
            const auto& tex = handle.getResource();
            vkapi::PipelineCache::DescriptorImage& image = storageImages[idx];
            image.imageView = tex->getImageView()->get();
            image.imageLayout = tex->getImageLayout();
        }
    }
    pipelineCache_->bindStorageImage(storageImages);

    // image samplers
    vkapi::PipelineCache::DescriptorImage imageSamplers[vkapi::PipelineCache::MaxSamplerBindCount];
    for (int idx = 0; idx < vkapi::PipelineCache::MaxSamplerBindCount; ++idx)
    {
        const vkapi::TextureHandle& handle = bundle->imageSamplers_[idx].texHandle;
        vk::Sampler sampler = bundle->imageSamplers_[idx].sampler;

        if (handle)
        {
            const auto& tex = handle.getResource();
            vkapi::PipelineCache::DescriptorImage& image = imageSamplers[idx];
            image.imageSampler = sampler;
            image.imageView = tex->getImageView()->get();
            image.imageLayout = tex->getImageLayout();
        }
    }
    pipelineCache_->bindSampler(imageSamplers);

    // Bind all the buffers associated with this pipeline
    for (const auto& info : bundle->descBindInfo_)
    {
        if (info.type == vk::DescriptorType::eUniformBuffer)
        {
            pipelineCache_->bindUbo(info.binding, info.buffer, info.size);
        }
        else if (info.type == vk::DescriptorType::eStorageBuffer)
        {
            pipelineCache_->bindSsbo(info.binding, info.buffer, info.size);
        }
    }
    plineLayout.build(context());
    pipelineCache_->bindDescriptors(cmd, plineLayout, {}, vk::PipelineBindPoint::eCompute);
    pipelineCache_->bindComputeShaderModules(*bundle);

    pipelineCache_->bindComputePipeline(cmd, plineLayout);

    // Bind the push block.
    size_t computeStage = util::ecast(backend::ShaderStage::Compute);
    if (bundle->pushBlock_[computeStage])
    {
        ASSERT_FATAL(
            bundle->pushBlock_[computeStage]->data, "No data has been set for this pushblock.");
        plineLayout.bindPushBlock(cmd, *bundle->pushBlock_[computeStage]);
    }

    cmd.dispatch(xWorkCount, yWorkCount, zWorkCount);
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

void VkDriver::generateMipMaps(const TextureHandle& handle, const vk::CommandBuffer& cmdBuffer)
{
    const Texture* texture = handle.getResource();
    const TextureContext& texParams = texture->context();

    ASSERT_LOG(texParams.width > 0 && texParams.height > 0);
    ASSERT_LOG(texParams.width == texParams.height);

    if (texParams.mipLevels == 1 || (texParams.width == 2 && texParams.height == 2))
    {
        return;
    }

    Image* image = texture->getImage();

    Image::transition(
        *image,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageLayout::eTransferSrcOptimal,
        cmdBuffer,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        0);

    for (uint8_t i = 1; i < texParams.mipLevels; ++i)
    {
        // source
        vk::ImageSubresourceLayers src(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
        vk::Offset3D srcOffset(texParams.width >> (i - 1), texParams.height >> (i - 1), 1);

        // destination
        vk::ImageSubresourceLayers dst(vk::ImageAspectFlagBits::eColor, i, 0, 1);
        vk::Offset3D dstOffset(texParams.width >> i, texParams.height >> i, 1);

        vk::ImageBlit imageBlit;
        imageBlit.srcSubresource = src;
        imageBlit.srcOffsets[1] = srcOffset;
        imageBlit.dstSubresource = dst;
        imageBlit.dstOffsets[1] = dstOffset;

        // create image barrier - transition image to transfer
        Image::transition(
            *image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            cmdBuffer,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands,
            i);

        // blit the image
        cmdBuffer.blitImage(
            image->get(),
            vk::ImageLayout::eTransferSrcOptimal,
            image->get(),
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &imageBlit,
            vk::Filter::eLinear);

        Image::transition(
            *image,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            cmdBuffer,
            vk::PipelineStageFlagBits::eAllCommands,
            vk::PipelineStageFlagBits::eAllCommands,
            i);
    }

    // prepare for shader read
    Image::transition(
        *image,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        cmdBuffer);
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
    auto* tex = const_cast<Texture*>(handle.getResource());
    tex->map(*this, data, dataSize, offsets);
}

void VkDriver::destroyTexture2D(TextureHandle& handle) { resourceCache_->deleteTexture(handle); }

void VkDriver::destroyBuffer(BufferHandle& handle) { resourceCache_->deleteUbo(handle); }

void VkDriver::deleteRenderTarget(const RenderTargetHandle& rtHandle)
{
    ASSERT_FATAL(
        rtHandle.getKey() < renderTargets_.size(), "Render target handle is out of range.");
    renderTargets_.erase(renderTargets_.begin() + rtHandle.getKey());
}

void VkDriver::collectGarbage() noexcept
{
    gc.collectGarbage();
    framebufferCache_->cleanCache(currentFrame_);
    pipelineCache_->cleanCache(currentFrame_);
    resourceCache_->garbageCollection();
    stagingPool_->garbageCollection(currentFrame_);
}

} // namespace vkapi
