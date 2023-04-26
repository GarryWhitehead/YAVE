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

#include "texture.h"

#include "commands.h"
#include "driver.h"
#include "image.h"
#include "utility.h"
#include "utility/assertion.h"

#include <spdlog/spdlog.h>


namespace vkapi
{

enum Filter
{
    Nearest,
    Linear,
    Cubic
};

enum AddressMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge
};

Texture::Texture(VkContext& context) : context_(context) {}
Texture::~Texture() {}

uint32_t Texture::getFormatCompSize(vk::Format format)
{
    uint32_t comp = 0;
    switch (format)
    {
        case vk::Format::eR8Snorm:
        case vk::Format::eR8Unorm:
        case vk::Format::eR8Sint:
        case vk::Format::eR8Srgb:
        case vk::Format::eR8Sscaled:
        case vk::Format::eR16Snorm:
        case vk::Format::eR16Sint:
        case vk::Format::eR16Sscaled:
        case vk::Format::eR16Sfloat:
        case vk::Format::eR32Sint:
        case vk::Format::eR32Sfloat:
            comp = 1;
            break;
        case vk::Format::eR8G8Snorm:
        case vk::Format::eR8G8Unorm:
        case vk::Format::eR8G8Sint:
        case vk::Format::eR8G8Srgb:
        case vk::Format::eR8G8Sscaled:
        case vk::Format::eR16G16Snorm:
        case vk::Format::eR16G16Sint:
        case vk::Format::eR16G16Sscaled:
        case vk::Format::eR16G16Sfloat:
        case vk::Format::eR32G32Sint:
        case vk::Format::eR32G32Sfloat:
            comp = 2;
            break;
        case vk::Format::eR8G8B8Snorm:
        case vk::Format::eR8G8B8Unorm:
        case vk::Format::eR8G8B8Sint:
        case vk::Format::eR8G8B8Srgb:
        case vk::Format::eR8G8B8Sscaled:
        case vk::Format::eR16G16B16Snorm:
        case vk::Format::eR16G16B16Sint:
        case vk::Format::eR16G16B16Sscaled:
        case vk::Format::eR16G16B16Sfloat:
        case vk::Format::eR32G32B32Sint:
        case vk::Format::eR32G32B32Sfloat:
            comp = 3;
            break;
        case vk::Format::eR8G8B8A8Snorm:
        case vk::Format::eR8G8B8A8Unorm:
        case vk::Format::eR8G8B8A8Sint:
        case vk::Format::eR8G8B8A8Srgb:
        case vk::Format::eR8G8B8A8Sscaled:
        case vk::Format::eR16G16B16A16Snorm:
        case vk::Format::eR16G16B16A16Sint:
        case vk::Format::eR16G16B16A16Sscaled:
        case vk::Format::eR16G16B16A16Sfloat:
        case vk::Format::eR32G32B32A32Sint:
        case vk::Format::eR32G32B32A32Sfloat:
            comp = 4;
            break;
    }
    return comp;
}

uint32_t Texture::getFormatByteSize(vk::Format format)
{
    uint32_t output = 1;
    switch (format)
    {
        case vk::Format::eR8Snorm:
        case vk::Format::eR8Unorm:
        case vk::Format::eR8Sint:
        case vk::Format::eR8Srgb:
        case vk::Format::eR8Sscaled:
        case vk::Format::eR8G8Snorm:
        case vk::Format::eR8G8Unorm:
        case vk::Format::eR8G8Sint:
        case vk::Format::eR8G8Srgb:
        case vk::Format::eR8G8Sscaled:
        case vk::Format::eR8G8B8Snorm:
        case vk::Format::eR8G8B8Unorm:
        case vk::Format::eR8G8B8Sint:
        case vk::Format::eR8G8B8Srgb:
        case vk::Format::eR8G8B8Sscaled:
        case vk::Format::eR8G8B8A8Snorm:
        case vk::Format::eR8G8B8A8Unorm:
        case vk::Format::eR8G8B8A8Sint:
        case vk::Format::eR8G8B8A8Srgb:
        case vk::Format::eR8G8B8A8Sscaled:
            output = 1;
            break;
        case vk::Format::eR16Snorm:
        case vk::Format::eR16Sint:
        case vk::Format::eR16Sscaled:
        case vk::Format::eR16Sfloat:
        case vk::Format::eR16G16Snorm:
        case vk::Format::eR16G16Sint:
        case vk::Format::eR16G16Sscaled:
        case vk::Format::eR16G16Sfloat:
        case vk::Format::eR16G16B16Snorm:
        case vk::Format::eR16G16B16Sint:
        case vk::Format::eR16G16B16Sscaled:
        case vk::Format::eR16G16B16Sfloat:
        case vk::Format::eR16G16B16A16Snorm:
        case vk::Format::eR16G16B16A16Sint:
        case vk::Format::eR16G16B16A16Sscaled:
        case vk::Format::eR16G16B16A16Sfloat:
            output = 4;
            break;
        case vk::Format::eR32Sint:
        case vk::Format::eR32G32Sint:
        case vk::Format::eR32G32B32Sint:
        case vk::Format::eR32G32B32A32Sint:
        case vk::Format::eR32Sfloat:
        case vk::Format::eR32G32Sfloat:
        case vk::Format::eR32G32B32Sfloat:
        case vk::Format::eR32G32B32A32Sfloat:
            output = 8;
            break;
        default:
            SPDLOG_WARN("Unsupported texture format - can not determine byte "
                        "size. Setting to one.");
            break;
    }
    return output;
}

void Texture::destroy() const
{
    if (image_)
    {
        image_->destroy();
    }
    if (imageView_)
    {
        context_.device().destroyImageView(imageView_->get(), nullptr);
    }
}

void Texture::createTexture2d(
    VkDriver& driver,
    vk::Format format,
    uint32_t width,
    uint32_t height,
    uint8_t mipLevels,
    uint8_t faceCount,
    uint8_t arrayCount,
    vk::ImageUsageFlags usageFlags)
{
    texContext_ = TextureContext {format, width, height, mipLevels, faceCount, arrayCount};

    // create an empty image
    image_ = std::make_unique<Image>(driver.context(), *this);
    image_->create(driver.vmaAlloc(), usageFlags);

    // and a image view of the empty image
    imageView_ = std::make_unique<ImageView>(driver.context());
    imageView_->create(driver.context().device(), *image_);

    if (isDepth(format) || isStencil(format))
    {
        imageLayout_ = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    }
    else
    {
        imageLayout_ = vk::ImageLayout::eShaderReadOnlyOptimal;
    }
}

void Texture::createTexture2d(
    VkDriver& driver, vk::Format format, uint32_t width, uint32_t height, vk::Image image)
{
    image_ = std::make_unique<Image>(driver.context(), image, format, width, height);
    texContext_ = TextureContext {format, width, height, 1, 1, 0};
    imageView_ = std::make_unique<ImageView>(driver.context());
    imageView_->create(driver.context().device(), image, format, 1, 1, 0);
    imageLayout_ = vk::ImageLayout::eShaderReadOnlyOptimal;
}

void Texture::map(VkDriver& driver, void* data, uint32_t dataSize, size_t* offsets)
{
    StagingPool stagePool = driver.stagingPool();
    StagingPool::StageInfo* stage = stagePool.create(dataSize);

    memcpy(stage->allocInfo.pMappedData, data, dataSize);
    vmaFlushAllocation(driver.vmaAlloc(), stage->mem, 0, dataSize);

    if (!offsets)
    {
        offsets = new size_t[texContext_.faceCount * texContext_.mipLevels];
        uint32_t offset = 0;
        for (uint32_t face = 0; face < texContext_.faceCount; face++)
        {
            for (uint32_t level = 0; level < texContext_.mipLevels; ++level)
            {
                offsets[face * texContext_.mipLevels + level] = offset;
                offset += (texContext_.width >> level) * (texContext_.height >> level) *
                    getFormatCompSize(texContext_.format) * getFormatByteSize(texContext_.format);
            }
        }
    }

    // create the info required for the copy
    std::vector<vk::BufferImageCopy> copyBuffers;
    for (uint32_t face = 0; face < texContext_.faceCount; ++face)
    {
        for (uint32_t level = 0; level < texContext_.mipLevels; ++level)
        {
            vk::BufferImageCopy imageCopy(
                offsets[face * texContext_.mipLevels + level],
                0,
                0,
                {vk::ImageAspectFlagBits::eColor, level, face, 1},
                {0, 0, 0},
                {texContext_.width >> level, texContext_.height >> level, 1});
            copyBuffers.emplace_back(imageCopy);
        }
    }

    // now copy image to local device - first prepare the image for copying via
    // transitioning to a transfer state. After copying, the image is
    // transistioned ready for reading by the shader
    auto& cmds = driver.getCommands();
    auto& cBuffer = cmds.getCmdBuffer();

    Image::transition(
        *image_,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        cBuffer.cmdBuffer);

    cBuffer.cmdBuffer.copyBufferToImage(
        stage->buffer,
        image_->get(),
        vk::ImageLayout::eTransferDstOptimal,
        static_cast<uint32_t>(copyBuffers.size()),
        copyBuffers.data());

    // the final transition here may need improving on....
    Image::transition(
        *image_, vk::ImageLayout::eTransferDstOptimal, imageLayout_, cBuffer.cmdBuffer);
}

const TextureContext& Texture::context() const { return texContext_; }

ImageView* Texture::getImageView() const
{
    ASSERT_LOG(imageView_);
    return imageView_.get();
}

Image* Texture::getImage() const
{
    ASSERT_LOG(image_);
    return image_.get();
}

const vk::ImageLayout& Texture::getImageLayout() const { return imageLayout_; }

} // namespace vkapi
