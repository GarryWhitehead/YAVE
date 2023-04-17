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

#include "image.h"

#include "commands.h"
#include "context.h"
#include "driver.h"
#include "utility/assertion.h"
#include "utility/logger.h"

namespace vkapi
{

// ================ ImageView =============================

ImageView::ImageView(const VkContext& context) : device_(context.device()) {}

ImageView::~ImageView() { device_.destroy(imageView_, nullptr); }

vk::ImageAspectFlags ImageView::getImageAspect(vk::Format format)
{
    vk::ImageAspectFlags aspect;

    switch (format)
    {
        // depth/stencil image formats
        case vk::Format::eD32SfloatS8Uint:
        case vk::Format::eD24UnormS8Uint:
            aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            break;
            // depth only formats
        case vk::Format::eD32Sfloat:
        case vk::Format::eD16Unorm:
            aspect = vk::ImageAspectFlagBits::eDepth;
            break;
            // otherwise, must be a colour format
        default:
            aspect = vk::ImageAspectFlagBits::eColor;
    }
    return aspect;
}

vk::ImageViewType ImageView::getTextureType(uint32_t faceCount, uint32_t arrayCount)
{
    if (arrayCount > 1 && faceCount == 1)
    {
        return vk::ImageViewType::e2DArray; // only dealing with 2d textures at
                                            // the moment
    }
    else if (faceCount == 6)
    {
        if (arrayCount == 1)
        {
            return vk::ImageViewType::eCube;
        }
        else
        {
            return vk::ImageViewType::eCubeArray;
        }
    }

    return vk::ImageViewType::e2D;
}

void ImageView::create(
    const vk::Device& dev,
    const vk::Image& image,
    vk::Format format,
    uint8_t faceCount,
    uint8_t mipLevels,
    uint8_t arrayCount)
{
    device_ = dev;

    vk::ImageViewType type = getTextureType(faceCount, arrayCount);

    // making assumptions here based on the image format used
    vk::ImageAspectFlags aspect = getImageAspect(format);

    vk::ImageViewCreateInfo createInfo(
        {},
        image,
        type,
        format,
        {vk::ComponentSwizzle::eIdentity,
         vk::ComponentSwizzle::eIdentity,
         vk::ComponentSwizzle::eIdentity,
         vk::ComponentSwizzle::eIdentity},
        vk::ImageSubresourceRange(aspect, 0, mipLevels, 0, faceCount));

    VK_CHECK_RESULT(device_.createImageView(&createInfo, nullptr, &imageView_));
}

void ImageView::create(const vk::Device& dev, const Image& image)
{
    create(
        dev,
        image.get(),
        image.context().format,
        image.context().faceCount,
        image.context().mipLevels,
        image.context().arrayCount);
}

// ==================== Image ===================

Image::Image(VkContext& context, const Texture& tex)
    : context_(context), device_(context.device()), tex_(tex.context())
{
}

Image::Image(
    VkContext& context,
    const vk::Image& image,
    const vk::Format& format,
    uint32_t width,
    uint32_t height)
    : context_(context), device_(context.device()), image_(image)
{
    tex_.format = format;
    tex_.width = width;
    tex_.height = height;
}

Image::~Image() {}

void Image::destroy() noexcept
{
    context_.device().freeMemory(imageMem_);
    context_.device().destroyImage(image_, nullptr);
}

vk::Filter Image::getFilterType(const vk::Format& format)
{
    vk::Filter filter;

    if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint ||
        format == vk::Format::eD32Sfloat || format == vk::Format::eD16Unorm)
    {
        filter = vk::Filter::eNearest;
    }
    else
    {
        filter = vk::Filter::eLinear;
    }
    return filter;
}

void Image::create(const VmaAllocator& vmaAlloc, vk::ImageUsageFlags usageFlags)
{
    ASSERT_LOG(tex_.format != vk::Format::eUndefined);

    vk::ImageCreateInfo imageInfo = {
        {},
        vk::ImageType::e2D, // TODO: support 3d images
        tex_.format,
        {tex_.width, tex_.height, 1},
        tex_.mipLevels,
        tex_.faceCount,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | usageFlags,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::ImageLayout::eUndefined};

    if (tex_.faceCount == 6)
    {
        imageInfo.arrayLayers = 6;
        imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
    }

    VK_CHECK_RESULT(device_.createImage(&imageInfo, nullptr, &image_));

    // allocate memory for the image....
    vk::MemoryRequirements memReq = {};
    device_.getImageMemoryRequirements(image_, &memReq);
    vk::MemoryAllocateInfo allocInfo = {
        memReq.size,
        context_.selectMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)};

    VK_CHECK_RESULT(device_.allocateMemory(&allocInfo, nullptr, &imageMem_));

    // and bind the image to the allocated memory.
    device_.bindImageMemory(image_, imageMem_, 0);
}

void Image::transition(
    const Image& image,
    const vk::ImageLayout& oldLayout,
    const vk::ImageLayout& newLayout,
    const vk::CommandBuffer& cmdBuff,
    uint32_t baseMipMapLevel)
{
    const TextureContext& tex = image.context();

    vk::ImageAspectFlags mask = ImageView::getImageAspect(tex.format);

    vk::AccessFlags srcBarrier, dstBarrier;

    switch (oldLayout)
    {
        case vk::ImageLayout::eUndefined:
            srcBarrier = (vk::AccessFlagBits)0;
            break;
        case vk::ImageLayout::eTransferDstOptimal:
            srcBarrier = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            srcBarrier = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            srcBarrier = vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        default:
            srcBarrier = (vk::AccessFlagBits)0;
    }

    switch (newLayout)
    {
        case vk::ImageLayout::eTransferDstOptimal:
            dstBarrier = vk::AccessFlagBits::eTransferWrite;
            break;
        case vk::ImageLayout::eTransferSrcOptimal:
            dstBarrier = vk::AccessFlagBits::eTransferRead;
            break;
        case vk::ImageLayout::eColorAttachmentOptimal:
            dstBarrier = vk::AccessFlagBits::eColorAttachmentWrite;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            dstBarrier = vk::AccessFlagBits::eShaderRead;
            break;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            dstBarrier = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            break;
        default:
            dstBarrier = (vk::AccessFlagBits)0;
    }

    vk::ImageSubresourceRange subresourceRange(
        mask, 0, tex.mipLevels, 0, tex.arrayCount * tex.faceCount);

    if (baseMipMapLevel != UINT32_MAX)
    {
        subresourceRange.baseMipLevel = baseMipMapLevel;
        subresourceRange.levelCount = 1;
    }

    vk::ImageMemoryBarrier memoryBarrier(
        srcBarrier,
        dstBarrier,
        oldLayout,
        newLayout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image.get(),
        subresourceRange);

    cmdBuff.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        (vk::DependencyFlags)0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &memoryBarrier);
}

void Image::generateMipMap(const Image& image, const vk::CommandBuffer& cmdBuffer)
{
    const TextureContext& tex = image.context();

    for (uint8_t i = 1; i < tex.mipLevels; ++i)
    {
        // source
        vk::ImageSubresourceLayers src(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
        vk::Offset3D srcOffset(tex.width >> (i - 1), tex.height >> (i - 1), 1);

        // destination
        vk::ImageSubresourceLayers dst(vk::ImageAspectFlagBits::eColor, i, 0, 1);
        vk::Offset3D dstOffset(tex.width >> i, tex.height >> i, 1);

        vk::ImageBlit imageBlit;
        imageBlit.srcSubresource = src;
        imageBlit.srcOffsets[1] = srcOffset;
        imageBlit.dstSubresource = dst;
        imageBlit.dstOffsets[1] = dstOffset;

        // create image barrier - transition image to transfer
        transition(
            image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmdBuffer, i);

        // blit the image
        cmdBuffer.blitImage(
            image.get(),
            vk::ImageLayout::eTransferSrcOptimal,
            image.get(),
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &imageBlit,
            vk::Filter::eLinear);

        transition(
            image,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            cmdBuffer,
            i);
    }

    // prepare for shader read
    transition(
        image,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        cmdBuffer);
}

void Image::blit(const Image& srcImage, const Image& dstImage, Commands& cmds)
{
    // source
    vk::CommandBuffer cmdBuffer = cmds.getCmdBuffer().cmdBuffer;

    const TextureContext& tex = srcImage.context();
    vk::ImageAspectFlags imageAspect = ImageView::getImageAspect(tex.format);

    vk::ImageSubresourceLayers src(imageAspect, 0, 0, 1);
    vk::Offset3D srcOffset(tex.width, tex.height, 1);

    // destination
    vk::ImageSubresourceLayers dst(imageAspect, 0, 0, 1);
    vk::Offset3D dstOffset(tex.width, tex.height, 1);

    vk::ImageBlit imageBlit;
    imageBlit.srcSubresource = src;
    imageBlit.srcOffsets[1] = srcOffset;
    imageBlit.dstSubresource = dst;
    imageBlit.dstOffsets[1] = dstOffset;

    transition(
        srcImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmdBuffer);
    transition(
        dstImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal, cmdBuffer);

    // blit the image
    vk::Filter filter = getFilterType(tex.format);
    cmdBuffer.blitImage(
        dstImage.get(),
        vk::ImageLayout::eTransferSrcOptimal,
        srcImage.get(),
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &imageBlit,
        filter);

    transition(
        srcImage,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        cmdBuffer);
    transition(
        dstImage,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        cmdBuffer);
}
} // namespace vkapi
