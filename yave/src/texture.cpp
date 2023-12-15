#include "private/mapped_texture.h"

namespace yave
{

void Texture::setTexture(const Params& params, size_t* offsets) noexcept
{
    size_t bufferSize = params.bufferSize;
    if (!bufferSize)
    {
        bufferSize = static_cast<IMappedTexture*>(this)->totalTextureSize(
            params.width, params.height, 1, params.faces, params.levels, params.format);
    }

    static_cast<IMappedTexture*>(this)->setTexture(
        params.buffer,
        bufferSize,
        params.width,
        params.height,
        params.levels,
        params.faces,
        params.format,
        params.usageFlags,
        offsets);
}

void Texture::setEmptyTexture(
    uint32_t width,
    uint32_t height,
    TextureFormat format,
    uint32_t usageFlags,
    uint32_t levels,
    uint32_t faces) noexcept
{
    uint32_t bufferSize = static_cast<IMappedTexture*>(this)->totalTextureSize(width, height, 1, faces, levels, format);
    void* buffer = (uint8_t*)new uint8_t[bufferSize];
    ASSERT_LOG(buffer);

    // if there are more than one mip level, then assume that a call
    // to generateMipMaps will happen which requires the image to be
    // given a src usage
    if (levels > 1)
    {
        usageFlags |= backend::ImageUsage::Src;
    }

    static_cast<IMappedTexture*>(this)->setTexture(buffer, bufferSize, width, height, levels, faces, format, usageFlags, nullptr);
}

Texture::Params Texture::getTextureParams() noexcept
{
    return static_cast<IMappedTexture*>(this)->getTextureParams();
}

void Texture::generateMipMaps()
{
    static_cast<IMappedTexture*>(this)->generateMipMaps();
}

}