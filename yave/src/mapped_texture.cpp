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

#include "mapped_texture.h"

#include "backend/enums.h"
#include "engine.h"
#include "utility/assertion.h"
#include "utility/logger.h"
#include "yave/texture.h"

namespace yave
{

IMappedTexture::IMappedTexture(IEngine& engine) : engine_(engine), buffer_(nullptr) {}

IMappedTexture::~IMappedTexture() {}

uint32_t IMappedTexture::getFormatByteSize(backend::TextureFormat format)
{
    size_t output = 0;
    switch (format)
    {
        case backend::TextureFormat::R8:
        case backend::TextureFormat::RG8:
        case backend::TextureFormat::RGB8:
        case backend::TextureFormat::RGBA8:
            output = 1;
            break;
        case backend::TextureFormat::R16:
        case backend::TextureFormat::RG16:
        case backend::TextureFormat::RGB16:
        case backend::TextureFormat::RGBA16:
            output = 4;
            break;
        case backend::TextureFormat::R32:
        case backend::TextureFormat::RG32:
        case backend::TextureFormat::RGB32:
        case backend::TextureFormat::RGBA32:
            output = 8;
            break;
    }
    return output;
}

uint32_t IMappedTexture::totalTextureSize(
    uint32_t width,
    uint32_t height,
    uint32_t layerCount,
    uint32_t faceCount,
    uint32_t mipLevels,
    backend::TextureFormat format) noexcept
{
    size_t byteSize = getFormatByteSize(format);

    size_t totalSize = 0;
    for (int i = 0; i < mipLevels; ++i)
    {
        totalSize += ((width >> i) * (height >> i) * 4 * byteSize) * faceCount * layerCount;
    }
    return totalSize;
}

void IMappedTexture::setTextureI(
    void* buffer,
    uint32_t bufferSize,
    uint32_t width,
    uint32_t height,
    uint32_t levels,
    uint32_t faces,
    backend::TextureFormat format,
    size_t* offsets)
{
    ASSERT_FATAL(buffer, "Invalid buffer is nullptr.");
    auto& driver = engine_.driver();

    buffer_ = buffer;
    width_ = width;
    height_ = height;
    mipLevels_ = levels;
    faceCount_ = faces;
    format_ = backend::textureFormatToVk(format);

    // we assume here that the textures created here will be always be sampled.
    // TODO: should be user defined
    auto usageFlags = vk::ImageUsageFlagBits::eSampled;

    tHandle_ = driver.createTexture2d(format_, width, height, levels, faces, 1, usageFlags);
    driver.mapTexture(tHandle_, buffer, bufferSize, offsets);
}

void IMappedTexture::setTextureI(
    void* buffer,
    uint32_t width,
    uint32_t height,
    uint32_t levels,
    uint32_t faces,
    backend::TextureFormat format,
    size_t* offsets)
{
    uint32_t bufferSize = totalTextureSize(width, height, 1, faces, levels, format);
    setTextureI(buffer, bufferSize, width, height, levels, faces, format, offsets);
}

// ================================== client api ===========================

Texture::Texture() {}
Texture::~Texture() {}

void IMappedTexture::setTexture(const Texture::Descriptor& desc, size_t* offsets) noexcept
{
    setTextureI(
        desc.buffer,
        desc.bufferSize,
        desc.width,
        desc.height,
        desc.levels,
        desc.faces,
        desc.format,
        offsets);
}

Texture::Descriptor IMappedTexture::getTextureDescriptor() noexcept
{
    return {buffer_, width_, height_, mipLevels_, faceCount_};
}

} // namespace yave
