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

#include "asset_loader.h"

#include <utility/assertion.h>
#include <yave/engine.h>
#include <yave/texture.h>

#define STB_IMAGE_IMPLEMENTATION
#include <ktx.h>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include <filesystem>

namespace yave
{

AssetLoader::AssetLoader(Engine* engine) : engine_(engine) {}
AssetLoader::~AssetLoader() {}

uint32_t AssetLoader::compSizeFromFormat(backend::TextureFormat format)
{
    uint32_t output;
    switch (format)
    {
        case backend::TextureFormat::R16:
        case backend::TextureFormat::R32:
        case backend::TextureFormat::R8:
            output = 1;
            break;
        case backend::TextureFormat::RG16:
        case backend::TextureFormat::RG32:
        case backend::TextureFormat::RG8:
            output = 2;
            break;
        case backend::TextureFormat::RGB16:
        case backend::TextureFormat::RGB32:
        case backend::TextureFormat::RGB8:
            output = 3;
            break;
        case backend::TextureFormat::RGBA16:
        case backend::TextureFormat::RGBA32:
        case backend::TextureFormat::RGBA8:
            output = 4;
            break;
        default:
            output = 4;
    }
    return output;
}

Texture*
AssetLoader::loadFromFile(const std::filesystem::path& filePath, backend::TextureFormat format)
{
    std::filesystem::path assetPath = filePath;
    if (!assetFolder_.empty())
    {
        assetPath = assetFolder_ / filePath;
    }
    auto platformAssetPath = assetPath.make_preferred();

    Texture* tex = nullptr;
    if (platformAssetPath.extension() == ".ktx")
    {
        tex = parseKtxFile(platformAssetPath, format);
    }
    else if (platformAssetPath.extension() == ".png" || platformAssetPath.extension() == ".jpg")
    {
        tex = parseImageFile(platformAssetPath, format);
    }
    else
    {
        SPDLOG_ERROR(
            "Unrecognised image suffix type: {}. Unable to proceed.",
            platformAssetPath.extension().string().c_str());
    }

    return tex;
}

Texture*
AssetLoader::parseImageFile(const std::filesystem::path& filePath, backend::TextureFormat format)
{
    ImageParams params;

    uint32_t reqComp = compSizeFromFormat(format);
    if (reqComp < 3)
    {
        SPDLOG_ERROR("Only comp of 3 or 4 supported for .png and .jpg images.");
        return {};
    }

    int width, height, comp;
    params.data = stbi_load((const char*)filePath.c_str(), &width, &height, &comp, reqComp);

    if (!params.data)
    {
        return {};
    }

    // only images with four components supported, so convert to four
    if (comp == 3)
    {
        // create a new buffer to hold all four components
        uint8_t* newBuffer = new uint8_t[width * height * 4];

        size_t oldSize = width * height * 3;

        // the alpha channel will be empty (memset here?)
        memcpy(newBuffer, params.data, oldSize);

        // delete the old buffer now we are finished with it
        stbi_image_free(params.data);
        params.data = newBuffer;
    }
    params.dataSize = width * height * comp;

    Texture* tex = engine_->createTexture();
    Texture::Params texParams {
        params.data,
        params.dataSize,
        params.width,
        params.height,
        format,
        backend::ImageUsage::Sampled, // should be user defined
        params.mipLevels,
        params.faceCount};
    tex->setTexture(texParams);
    stbi_image_free(params.data);

    return tex;
}

Texture*
AssetLoader::parseKtxFile(const std::filesystem::path& filePath, backend::TextureFormat format)
{
    ImageParams params;

    ktxTexture* texture;
    KTX_error_code result;

    result = ktxTexture_CreateFromNamedFile(
        filePath.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

    if (result != KTX_error_code::KTX_SUCCESS)
    {
        SPDLOG_CRITICAL("Unable to open ktx image file {}.", filePath.string().c_str());
        return {};
    }

    ASSERT_FATAL(
        texture->numDimensions == 2, "Only 2D textures supported by the engine currently.");

    params.width = texture->baseWidth;
    params.height = texture->baseHeight;
    params.faceCount = texture->numFaces;
    params.mipLevels = texture->numLevels;
    params.arrayCount = texture->numLayers;
    params.data = texture->pData;
    params.dataSize = static_cast<uint32_t>(texture->dataSize);

    size_t* offsets = new size_t[params.faceCount * params.mipLevels];
    for (uint32_t face = 0; face < params.faceCount; face++)
    {
        for (uint32_t level = 0; level < params.mipLevels; ++level)
        {
            KTX_error_code ret = ktxTexture_GetImageOffset(
                texture, level, 0, face, &offsets[face * params.mipLevels + level]);
            ASSERT_FATAL(ret == KTX_SUCCESS, "Error whilst generating image offsets.");
        }
    }

    Texture* tex = engine_->createTexture();
    Texture::Params texParams {
        params.data,
        params.dataSize,
        params.width,
        params.height,
        format,
        backend::ImageUsage::Sampled, // should be user defined
        params.mipLevels,
        params.faceCount};
    tex->setTexture(texParams, offsets);

    ktxTexture_Destroy(texture);
    delete[] offsets;

    return tex;
}

void AssetLoader::setAssetFolder(const std::filesystem::path& assetPath)
{
    ASSERT_LOG(!assetPath.empty());
    assetFolder_ = assetPath;
}

} // namespace yave
