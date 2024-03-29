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

#include "object_instance.h"
#include "utility/assertion.h"

#include <unordered_map>
#include <vector>

namespace yave
{

class ComponentManager
{
public:
    static constexpr int MinimumFreeSlots = 1024;

    ComponentManager();
    ~ComponentManager();

    // component managers are neither copyable nor moveable
    ComponentManager(const ComponentManager&) = delete;
    ComponentManager& operator=(const ComponentManager&) = delete;

    /**
     * @brief Adds an Object to the list and returns its location
     * This will either be a new slot or an already created one if any
     * have been freed
     */
    ObjectHandle addObject(const Object& obj) noexcept;

    /**
     * @brief Returns an Objects index value if found
     * Note: returns zero if not found
     */
    ObjectHandle getObjIndex(const Object& obj);

    /**
     * @brief Removes an Object from the manager and adds its slot index to
     * to the freed list for reuse.
     */
    bool removeObject(const Object& obj);

    bool hasObject(const Object& obj);

    using ObjectMap = std::unordered_map<Object, size_t, ObjHash, ObjEqual>;

protected:
    // the Objects which contain this component and their index location
    ObjectMap objects_;

    // free buffer indices from destroyed Objects.
    // rather than resize buffers which will be slow, empty slots in manager
    // containers are stored here and re-used
    std::vector<size_t> freeSlots_;

    // the current index into the main manager buffers which will be allocated
    // to the next Object that is added.
    uint64_t index_;
};
} // namespace yave
