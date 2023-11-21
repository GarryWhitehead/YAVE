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

#include "object_manager.h"

#include "yave/object.h"

#include <cstring>

namespace yave
{

IObjectManager::IObjectManager() : currentIdx_(1)
{
    generations_ = new uint8_t[IndexCount];
    memset(generations_, 0, sizeof(uint8_t) * IndexCount);
}

IObjectManager::~IObjectManager() { delete[] generations_; }

Object IObjectManager::makeObject(uint8_t generation, uint32_t index)
{
    return ((generation << IndexBits) | index);
}

bool IObjectManager::isAlive(Object& obj) const noexcept
{
    return getGeneration(obj) == generations_[getIndex(obj)];
}

Object IObjectManager::createObjectI()
{
    uint64_t id = 0;
    if (!freeIds_.empty() && freeIds_.size() > MinimumFreeIds)
    {
        id = freeIds_.front();
        freeIds_.pop_front();
    }
    else
    {
        id = currentIdx_++;
    }

    return makeObject(generations_[id], id);
}

void IObjectManager::destroyObjectI(Object& obj)
{
    uint32_t index = getIndex(obj);
    freeIds_.push_back(index);
    generations_[index]++;
}

uint32_t IObjectManager::getIndex(const Object& obj) { return obj.getId() & IndexMask; }

uint8_t IObjectManager::getGeneration(const Object& obj)
{
    return (obj.getId() >> IndexBits) & GenerationMask;
}

// ================================ client api ============================

ObjectManager::ObjectManager() = default;
ObjectManager::~ObjectManager() = default;

Object IObjectManager::createObject() { return createObjectI(); }

void IObjectManager::destroyObject(Object& obj) { destroyObjectI(obj); }

} // namespace yave