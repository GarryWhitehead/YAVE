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

#include "utility/assertion.h"
#include "yave/object.h"

#include <cstdint>

namespace yave
{

class ObjectHandle
{
public:
    static constexpr uint64_t InvalidHandle = UINT64_MAX;

    ObjectHandle() : handle_(InvalidHandle) {}
    explicit ObjectHandle(uint64_t h) : handle_(h) {}

    explicit operator bool() const noexcept { return handle_ != InvalidHandle; }
    bool operator==(const ObjectHandle rhs) { return handle_ == rhs.handle_; }
    bool operator!=(const ObjectHandle rhs) { return handle_ != rhs.handle_; }

    uint64_t get() const noexcept
    {
        ASSERT_LOG(handle_ != InvalidHandle);
        return handle_;
    }

    void invalidate() noexcept { handle_ = InvalidHandle; }

    bool valid() const noexcept { return handle_ != InvalidHandle; }

private:
    uint64_t handle_;
};

class IObject : public Object
{
public:
    IObject(const uint64_t id) : id_(id) {}

    // operator overloads
    bool operator==(const IObject& obj) const noexcept
    {
        return id_ == obj.id_;
    }
    bool operator!=(const IObject& obj) const noexcept
    {
        return id_ != obj.id_;
    }
    bool operator>(const IObject& obj) const noexcept { return id_ > obj.id_; }
    bool operator>=(const IObject& obj) const noexcept
    {
        return id_ >= obj.id_;
    }
    bool operator<(const IObject& obj) const noexcept { return id_ < obj.id_; }
    bool operator<=(const IObject& obj) const noexcept
    {
        return id_ <= obj.id_;
    }

    // helper functions
    uint64_t id() const noexcept { return id_; }

    void setId(const uint64_t objId) noexcept { id_ = objId; }

    bool isActive() const noexcept { return active_; }

private:
    uint64_t id_;
    bool active_ = true;
};

/**
 * Hasher overloads to allow for IObjects to be hashed via their id
 */
struct ObjHash
{
    size_t operator()(const IObject& id) const
    {
        return std::hash<uint64_t> {}(id.id());
    }
};

struct ObjEqual
{
    bool operator()(const IObject& lhs, const IObject& rhs) const
    {
        return lhs.id() == rhs.id();
    }
};

} // namespace yave
