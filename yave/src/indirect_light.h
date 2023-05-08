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
#include "mapped_texture.h"
#include "yave/indirect_light.h"

namespace yave
{
class IIndirectLight : public IndirectLight
{
public:
    IIndirectLight();
    ~IIndirectLight();

    void setIrradianceMapI(IMappedTexture* cubeMap);
    void setSpecularMapI(IMappedTexture* specCubeMap, IMappedTexture* brdfLut);

    vkapi::TextureHandle getIrradianceMapHandle() noexcept;
    vkapi::TextureHandle getSpecularMapHandle() noexcept;
    vkapi::TextureHandle getBrdfLutHandle() noexcept;

    uint32_t getMipLevels() const noexcept { return mipLevels_; }

    // ====================== client api ===============================

    void setIrrandianceMap(Texture* irradianceMap) noexcept override;

    void setSpecularMap(Texture* specularMap, Texture* brdfLut) override;

private:
    IMappedTexture* irradianceMap_;
    IMappedTexture* specularMap_;
    IMappedTexture* brdfLut_;

    uint32_t mipLevels_;
};
} // namespace yave