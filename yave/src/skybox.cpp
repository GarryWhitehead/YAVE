#include "private/skybox.h"
#include "private/mapped_texture.h"
#include "private/scene.h"

namespace yave
{

void Skybox::setTexture(Texture* texture) noexcept
{
    static_cast<ISkybox*>(this)->setCubeMap(static_cast<IMappedTexture*>(texture));
}

void Skybox::build(Scene* scene)
{
    static_cast<ISkybox*>(this)->build(*static_cast<IScene*>(scene));
}

void Skybox::setColour(const util::Colour4& col) noexcept
{
    static_cast<ISkybox*>(this)->setColour(col);
}

void Skybox::renderSun(bool state) noexcept
{
    static_cast<ISkybox*>(this)->renderSun(state);
}

}