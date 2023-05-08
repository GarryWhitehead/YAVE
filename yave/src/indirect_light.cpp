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

#include "indirect_light.h"

#include "mapped_texture.h"

#include <utility/assertion.h>

namespace yave
{
IIndirectLight::IIndirectLight()
    : irradianceMap_(nullptr), specularMap_(nullptr), brdfLut_(nullptr), mipLevels_(0)
{
}
IIndirectLight::~IIndirectLight() {}

void IIndirectLight::setIrradianceMapI(IMappedTexture* cubeMap)
{
    ASSERT_FATAL(cubeMap, "Irradiance cube map is nullptr");
    ASSERT_FATAL(cubeMap->isCubeMap(), "The irradiance env map must be a cubemap");
    irradianceMap_ = cubeMap;
}

void IIndirectLight::setSpecularMapI(IMappedTexture* specCubeMap, IMappedTexture* brdfLut)
{
    ASSERT_FATAL(specCubeMap, "Specular cube map is nullptr");
    ASSERT_FATAL(brdfLut, "A valid pointer to a brdf LUT texture is required.");
    ASSERT_FATAL(specCubeMap->isCubeMap(), "The irradiance env map must be a cubemap");
    specularMap_ = specCubeMap;
    brdfLut_ = brdfLut;

    mipLevels_ = specularMap_->getMipLevels();
}

vkapi::TextureHandle IIndirectLight::getIrradianceMapHandle() noexcept
{
    ASSERT_LOG(irradianceMap_);
    return irradianceMap_->getBackendHandle();
}

vkapi::TextureHandle IIndirectLight::getSpecularMapHandle() noexcept
{
    ASSERT_LOG(specularMap_);
    return specularMap_->getBackendHandle();
}

vkapi::TextureHandle IIndirectLight::getBrdfLutHandle() noexcept
{
    ASSERT_LOG(brdfLut_);
    return brdfLut_->getBackendHandle();
}

// ======================== client api ============================

IndirectLight::IndirectLight() = default;
IndirectLight::~IndirectLight() = default;

void IIndirectLight::setIrrandianceMap(Texture* irradianceMap) noexcept
{
    setIrradianceMapI(reinterpret_cast<IMappedTexture*>(irradianceMap));
}

void IIndirectLight::setSpecularMap(Texture* specularMap, Texture* brdfLut)
{
    setSpecularMapI(
        reinterpret_cast<IMappedTexture*>(specularMap), reinterpret_cast<IMappedTexture*>(brdfLut));
}

} // namespace yave