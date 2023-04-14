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

#include "utility/assertion.h"
#include "utility/handle.h"
#include "vulkan-api/driver.h"
#include "yave/texture.h"

#include <filesystem>
#include <memory>
#include <vector>

namespace yave
{
class IEngine;

class IMappedTexture : public Texture
{
public:

    IMappedTexture(IEngine& engine);
    ~IMappedTexture();

    // textures are not copyable
    IMappedTexture(const IMappedTexture& other) = delete;
    IMappedTexture& operator=(const IMappedTexture& other) = delete;

    void setTextureI(
        void* buffer,
        uint32_t bufferSize,
        uint32_t width,
        uint32_t height,
        uint32_t levels,
        uint32_t faces,
        backend::TextureFormat type,
        size_t* offsets = nullptr);

    void setTextureI(
        void* buffer,
        uint32_t width,
        uint32_t height,
        uint32_t levels,
        uint32_t faces,
        backend::TextureFormat type,
        size_t* offsets = nullptr);

    static uint32_t totalTextureSize(
        uint32_t width,
        uint32_t height,
        uint32_t layerCount,
        uint32_t faceCount,
        uint32_t mipLevels,
        backend::TextureFormat format) noexcept;

    static uint32_t getFormatByteSize(backend::TextureFormat format);

    bool isCubeMap() const noexcept { return faceCount_ == 6; }

    // ================= getters =====================

    void* getBuffer() { return buffer_; }
    size_t getWidth() const noexcept { return width_; }
    size_t getHeight() const noexcept { return height_; }
    uint32_t getMipLevels() const noexcept { return mipLevels_; }
    uint32_t getFaceCount() const noexcept { return faceCount_; }
    vk::Format getFormat() const noexcept { return format_; }

    vkapi::TextureHandle& getBackendHandle() { return tHandle_; }

    // ================== client api ===================

    void setTexture(const Descriptor& desc, size_t* offsets) noexcept override;

    Descriptor getTextureDescriptor() noexcept override;

private:
    IEngine& engine_;

    // the mapped texture binary.
    void* buffer_;

    // vulkan info that is associated with this texture
    vk::Format format_;

    // dimensions of the image
    uint32_t width_;
    uint32_t height_;
    uint32_t mipLevels_;
    uint32_t faceCount_;

    // =========== vulkan backend ================
    vkapi::TextureHandle tHandle_;
};

} // namespace yave
