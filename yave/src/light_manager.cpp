#include "private/managers/light_manager.h"

namespace yave
{

void LightManager::create(const CreateInfo& ci, Type type, Object& obj)
{
    static_cast<ILightManager*>(this)->createLight(ci, obj, type);
}

void LightManager::setIntensity(float intensity, Object& obj)
{
    static_cast<ILightManager*>(this)->setIntensity(intensity, obj);
}

void LightManager::setFallout(float fallout, Object& obj)
{
    static_cast<ILightManager*>(this)->setFallout(fallout, obj);
}

void LightManager::setPosition(const mathfu::vec3& pos, Object& obj)
{
    static_cast<ILightManager*>(this)->setPosition(pos, obj);
}

void LightManager::setTarget(const mathfu::vec3& target, Object& obj)
{
    static_cast<ILightManager*>(this)->setTarget(target, obj);
}

void LightManager::setColour(const mathfu::vec3& col, Object& obj)
{
    static_cast<ILightManager*>(this)->setColour(col, obj);
}

void LightManager::setFov(float fov, Object& obj)
{
    static_cast<ILightManager*>(this)->setFov(fov, obj);
}

} // namespace yave