#include "private/managers/renderable_manager.h"

#include "private/renderable.h"
#include "private/scene.h"

namespace yave
{

void RenderableManager::build(
    Scene* scene,
    Renderable* renderable,
    Object& obj,
    const ModelTransform& transform,
    const std::string& matShader)
{
    static_cast<IRenderableManager*>(this)->build(
        *static_cast<IScene*>(scene),
        static_cast<IRenderable*>(renderable),
        obj,
        transform,
        matShader);
}

Material* RenderableManager::createMaterial() noexcept
{
    return static_cast<IRenderableManager*>(this)->createMaterial();
}

void RenderableManager::destroy(const Object& obj)
{
    static_cast<IRenderableManager*>(this)->destroy(obj);
}

void RenderableManager::destroy(Material* mat)
{
    static_cast<IRenderableManager*>(this)->destroy(static_cast<IMaterial*>(mat));
}

} // namespace yave