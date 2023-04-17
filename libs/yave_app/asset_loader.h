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

#include "utility/cstring.h"

#include <backend/enums.h>

#include <filesystem>

namespace yave
{
class Engine;
class Texture;

class AssetLoader
{
public:
    static constexpr int MaxFaceCount = 6;
    static constexpr int MaxMipLevelCount = 12;

    enum class ImageType
    {
        Ktx,
        Png,
        Jpeg
    };

    struct ImageParams
    {
        ImageType type;
        uint32_t width;
        uint32_t height;
        uint32_t comp;
        uint32_t faceCount = 1;
        uint32_t arrayCount = 1;
        uint32_t mipLevels = 1;
        uint32_t dataSize = 0;
        void* data = nullptr;
    };

    AssetLoader(Engine* engine);
    ~AssetLoader();

    AssetLoader(const AssetLoader&) = delete;
    AssetLoader& operator=(const AssetLoader&) = delete;
    AssetLoader(AssetLoader&&) = default;
    AssetLoader& operator=(AssetLoader&&) = default;

    static uint32_t compSizeFromFormat(backend::TextureFormat format);

    /**
     * loads a image file supported by stb (jpg, png, etc.) into a bufffer
     * @brief filename absolute path to desired image file
     * @return Whther the image was successfully mapped into the buffer
     */
    Texture* loadFromFile(const std::filesystem::path& filePath, backend::TextureFormat format);

    Texture* parseImageFile(const std::filesystem::path& filePath, backend::TextureFormat format);

    Texture* parseKtxFile(const std::filesystem::path& filePath, backend::TextureFormat format);

    void setAssetFolder(const std::filesystem::path& assetPath);

private:
    Engine* engine_;

    std::filesystem::path assetFolder_;
};

} // namespace yave