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

#include <cstdint>
#include <functional>

namespace yave
{

class Object
{
public:
    Object() : id_(0) {}
    Object(const uint64_t id) : id_(id) {}

    // operator overloads
    bool operator==(const Object& obj) const noexcept { return id_ == obj.id_; }
    bool operator!=(const Object& obj) const noexcept { return id_ != obj.id_; }
    bool operator>(const Object& obj) const noexcept { return id_ > obj.id_; }
    bool operator>=(const Object& obj) const noexcept { return id_ >= obj.id_; }
    bool operator<(const Object& obj) const noexcept { return id_ < obj.id_; }
    bool operator<=(const Object& obj) const noexcept { return id_ <= obj.id_; }

    // helper functions
    uint64_t getId() const noexcept { return id_; }

    void setId(const uint64_t objId) noexcept { id_ = objId; }

    // An id denotes an invalidated object as reserved by the object
    // manager for this purpose.
    bool isValid() const noexcept { return id_ != 0; }

private:
    uint64_t id_;
};

/**
 * Hasher overloads to allow for Objects to be hashed via their id
 */
struct ObjHash
{
    size_t operator()(const Object& id) const { return std::hash<uint64_t> {}(id.getId()); }
};

struct ObjEqual
{
    bool operator()(const Object& lhs, const Object& rhs) const
    {
        return lhs.getId() == rhs.getId();
    }
};

} // namespace yave
