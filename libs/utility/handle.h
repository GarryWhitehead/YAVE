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
#include <limits>

namespace util
{

class HandleBase
{
public:
    constexpr static uint32_t UNINITIALISED = std::numeric_limits<uint32_t>::max();

    explicit operator bool() const noexcept { return key_ != UNINITIALISED; }

    bool operator==(const HandleBase& rhs) const { return key_ == rhs.key_; }
    bool operator!=(const HandleBase& rhs) const { return key_ != rhs.key_; }
    bool operator<(const HandleBase& rhs) const { return key_ < rhs.key_; }
    bool operator>(const HandleBase& rhs) const { return key_ > rhs.key_; }
    bool operator<=(const HandleBase& rhs) const { return key_ <= rhs.key_; }
    bool operator>=(const HandleBase& rhs) const { return key_ >= rhs.key_; }

    HandleBase() : key_(UNINITIALISED) {}
    explicit HandleBase(uint32_t key) : key_(key) {}

    [[nodiscard]] uint32_t getKey() const noexcept { return key_; }

private:
    uint32_t key_;
};

template <typename T>
class Handle : public HandleBase
{
public:
    Handle() = default;
    explicit Handle(const uint32_t key) : HandleBase(key) {}

    // default copy constructors
    Handle(const Handle&) = default;
    Handle& operator=(const Handle&) = default;
};

} // namespace util
