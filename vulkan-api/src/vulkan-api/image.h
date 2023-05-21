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

#include "common.h"
#include "texture.h"
#include "utility/compiler.h"

#include <cstdint>

namespace vkapi
{
// forward declarations
class VkContext;
class Commands;
class Image;

class ImageView
{
public:
    ImageView(const VkContext& context);
    ~ImageView();

    static vk::ImageAspectFlags getImageAspect(vk::Format format);

    static vk::ImageViewType getTextureType(uint32_t faceCount, uint32_t arrayCount);

    void create(const vk::Device& dev, const Image& image, uint32_t level);

    void create(
        const vk::Device& dev,
        const vk::Image& image,
        vk::Format format,
        uint8_t faceCount,
        uint8_t mipLevels,
        uint8_t arrayCount,
        uint32_t level);

    const vk::ImageView& get() const { return imageView_; }

private:
    vk::Device device_;
    vk::ImageView imageView_;
};

class Image
{
public:
    Image(
        VkContext& context,
        const vk::Image& image,
        const vk::Format& format,
        uint32_t width,
        uint32_t height);
    Image(VkContext& context, const Texture& tex);
    ~Image();

    void destroy() noexcept;

    static vk::Filter getFilterType(const vk::Format& format);

    void create(const VmaAllocator& vmaAlloc, vk::ImageUsageFlags usageFlags);

    static void transition(
        const Image& image,
        const vk::ImageLayout& oldLayout,
        const vk::ImageLayout& newLayout,
        const vk::CommandBuffer& cmdBuff,
        vk::PipelineStageFlags srcStage = vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlags dstStage = vk::PipelineStageFlagBits::eAllCommands,
        uint32_t baseMipMapLevel = UINT32_MAX);

    static void blit(const Image& srcImage, const Image& dstImage, Commands& cmd);

    const vk::Image& get() const { return image_; }

    const TextureContext& context() const { return tex_; }

private:
    VkContext& context_;
    vk::Device device_;
    TextureContext tex_;
    vk::Image image_;
    vk::DeviceMemory imageMem_;
};

} // namespace vkapi
