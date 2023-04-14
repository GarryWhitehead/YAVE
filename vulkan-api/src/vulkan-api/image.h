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
    /// no default contructor
    ImageView(const VkContext& context);
    ~ImageView();

    /**
     * @brief Returns the aspect flags bsed on the texture format
     */
    static vk::ImageAspectFlags getImageAspect(vk::Format format);

    /**
     * @brief Calculates the view type based on how many faces and whether the
     * texture is an array
     */
    static vk::ImageViewType
    getTextureType(uint32_t faceCount, uint32_t arrayCount);

    /**
     * @brief Create a new image view based on the specified **Image**
     */
    void create(const vk::Device& dev, const Image& image);

    void create(
        const vk::Device& dev,
        const vk::Image& image,
        vk::Format format,
        uint8_t faceCount,
        uint8_t mipLevels,
        uint8_t arrayCount);

    /**
     * @brief Return the vulkan image view handle
     */
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

    /**
     * Returns the interpolation filter based on the format type
     */
    static vk::Filter getFilterType(const vk::Format& format);

    /**
     * @brief Create a new VkImage instance based on the specified texture  and
     * usage flags
     */
    void create(const VmaAllocator& vmaAlloc, vk::ImageUsageFlags usageFlags);

    /**
     *@brief Tansitions the image from one layout to another.
     */
    static void transition(
        const Image& image,
        const vk::ImageLayout& oldLayout,
        const vk::ImageLayout& newLayout,
        const vk::CommandBuffer& cmdBuff,
        uint32_t baseMipMapLevel = UINT32_MAX);

    // =========== static functions ===================
    /**
     * @brief Generates mip maps for the required levels for this image
     */
    void generateMipMap(const Image& image, const vk::CommandBuffer& cmdBuffer);

    /**
     * @brief Blits the source image to the dst image using the specified
     */
    static void blit(const Image& srcImage, const Image& dstImage, Commands& cmd);

    /**
     * @brief Returns the vulkan image handle
     */
    const vk::Image& get() const { return image_; }

    /**
     * @brief Returns the texture context associated with this image
     */
    const TextureContext& context() const { return tex_; }

private:
    VkContext& context_;
    vk::Device device_;
    TextureContext tex_;
    vk::Image image_;
    vk::DeviceMemory imageMem_;
};

} // namespace vkapi
