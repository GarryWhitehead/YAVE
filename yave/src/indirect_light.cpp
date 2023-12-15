#include "private/indirect_light.h"

namespace yave
{

void IndirectLight::setIrrandianceMap(Texture* irradianceMap) noexcept
{
    static_cast<IIndirectLight*>(this)->setIrradianceMap(
        static_cast<IMappedTexture*>(irradianceMap));
}

void IndirectLight::setSpecularMap(Texture* specularMap, Texture* brdfLut)
{
    static_cast<IIndirectLight*>(this)->setSpecularMap(
        static_cast<IMappedTexture*>(specularMap), static_cast<IMappedTexture*>(brdfLut));
}

} // namespace yave