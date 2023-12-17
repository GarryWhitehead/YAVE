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

#include "yave_api.h"

#include <backend/enums.h>

namespace yave
{

class Texture : public YaveApi
{
public:
    using TextureFormat = backend::TextureFormat;
    using ImageUsage = backend::ImageUsage;

    struct Params
    {
        void* buffer = nullptr;
        size_t bufferSize = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        TextureFormat format = TextureFormat::Undefined;
        uint32_t usageFlags = 0;
        uint32_t levels = 1;
        uint32_t faces = 1;
    };

    void setTexture(const Params& params, size_t* offsets = nullptr) noexcept;

    void setEmptyTexture(
        uint32_t width,
        uint32_t height,
        TextureFormat format,
        uint32_t usageFlags,
        uint32_t levels = 1,
        uint32_t faces = 1) noexcept;

    Params getTextureParams() noexcept;

    void generateMipMaps();

protected:
    Texture() = default;
    ~Texture() = default;
};

} // namespace yave
