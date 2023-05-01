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

#include "component_manager.h"

namespace yave
{

ComponentManager::ComponentManager() : index_(0) {}
ComponentManager::~ComponentManager() {}

ObjectHandle ComponentManager::addObject(const Object& obj) noexcept
{
    uint64_t retIdx = 0;

    // if there are free slots and the threshold has been reached,
    // use an empty slot.
    if (!freeSlots_.empty() && freeSlots_.size() > MinimumFreeSlots)
    {
        retIdx = freeSlots_.back();
        objects_.emplace(obj, retIdx);
        freeSlots_.pop_back();
    }
    else
    {
        objects_.emplace(obj, index_);
        retIdx = index_++;
    }
    return ObjectHandle(retIdx);
}

ObjectHandle ComponentManager::getObjIndex(const Object& obj)
{
    auto iter = objects_.find(obj);
    if (iter == objects_.end())
    {
        return {};
    }
    return ObjectHandle(iter->second);
}

bool ComponentManager::hasObject(const Object& obj)
{
    if (objects_.find(obj) == objects_.end())
    {
        return false;
    }
    return true;
}

bool ComponentManager::removeObject(const Object& obj)
{
    auto iter = objects_.find(obj);
    if (iter == objects_.end())
    {
        return false;
    }
    freeSlots_.emplace_back(iter->second);
    objects_.erase(iter);

    return true;
}

} // namespace yave
