#include "private/managers/transform_manager.h"

namespace yave
{

void TransformManager::addModelTransform(const ModelTransform& transform, Object& obj)
{
    mathfu::mat4 r = transform.rot.ToMatrix4();
    mathfu::mat4 s = mathfu::mat4::FromScaleVector(transform.scale);
    mathfu::mat4 t = mathfu::mat4::FromTranslationVector(transform.translation);
    mathfu::mat4 local = t * r * s;
    static_cast<ITransformManager*>(this)->addTransform(local, obj);
}

}