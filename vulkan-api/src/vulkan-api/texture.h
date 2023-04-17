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
#include "utility/compiler.h"

#include <memory>

namespace vkapi
{
// forward declerations
class Image;
class ImageView;
class VkDriver;
class VkContext;
class VkDriver;

/**
 * A simple struct for storing all texture info and ease of passing around
 */
struct TextureContext
{
    vk::Format format;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    uint32_t faceCount = 1;
    uint32_t arrayCount = 1;
};

class Texture
{
public:
    Texture(VkContext& context);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    static uint32_t getFormatByteSize(vk::Format format);
    static uint32_t getFormatCompSize(vk::Format format);

    /**
     * @brief Creates a image and image view for a 2d texture
     * @param format The format of the texture - see **vk::Format**
     * @param width The width of the texture in pixels
     * @param height The height of the texture in pixels
     * @param mipLevels The number of levels this texture contains
     * @param usageFlags The intended usage for this image.
     */
    void createTexture2d(
        VkDriver& driver,
        vk::Format format,
        uint32_t width,
        uint32_t height,
        uint8_t mipLevels,
        uint8_t faceCount,
        uint8_t arrayCount,
        vk::ImageUsageFlags usageFlags);

    void createTexture2d(
        VkDriver& driver, vk::Format format, uint32_t width, uint32_t height, vk::Image image);

    void destroy() const;

    /**
     * @brief Maps a image in the format specified when the texture as created,
     * to the GPU.
     */
    void map(VkDriver& driver, void* data, uint32_t dataSize, size_t* offsets);

    // ================= getters =======================
    ImageView* getImageView() const;
    Image* getImage() const;
    const vk::ImageLayout& getImageLayout() const;

    /**
     * @brief Returns a struct containing all user relevant info for this
     * texture
     */
    const TextureContext& context() const;

    friend class ResourceCache;

private:
    // texture info
    VkContext& context_;

    TextureContext texContext_;

    vk::ImageLayout imageLayout_ = vk::ImageLayout::eUndefined;

    std::unique_ptr<Image> image_;
    std::unique_ptr<ImageView> imageView_;

    uint64_t framesUntilGc;
};

} // namespace vkapi
