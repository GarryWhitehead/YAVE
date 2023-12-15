#include "private/object_manager.h"

namespace yave
{

Object ObjectManager::createObject()
{
    return static_cast<IObjectManager*>(this)->createObject();
}

void ObjectManager::destroyObject(Object& obj)
{
    static_cast<IObjectManager*>(this)->destroyObject(obj);
}

}