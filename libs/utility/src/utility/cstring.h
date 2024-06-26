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

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <vector>

namespace util
{

/**
 A simple class that wraps a c style string to make easier to use.
 */
class CString
{
public:
    CString() = default;
    CString(const char* str);

    CString(const CString& str);
    CString& operator=(const CString& str);

    CString(CString&& str) noexcept;
    CString& operator=(CString&& str) noexcept;

    bool operator==(const CString& rhs) const noexcept;
    bool operator!=(const CString& rhs) const noexcept;

    char& operator[](size_t idx) noexcept;
    char& operator[](size_t idx) const noexcept;

    ~CString();

    /**
     * @brief A compare two CStrings; a simple wrapper around the c-style
     * strncmp
     * @param str the CString to compare with
     * @return A boolean indicating whether the two CStrings are identical
     */
    [[nodiscard]] bool compare(const CString& str) const;

    [[nodiscard]] bool empty() const { return !buffer; }

    [[nodiscard]] size_t size() const { return length; }

    [[nodiscard]] char* c_str() const { return buffer; }

    // ================== static functions ==========================
    static std::vector<CString> split(const CString& str, char identifier);

    /**
     * @brief Appends a CString to the end of the CString held by the 'this'
     * buffer
     */
    static util::CString append(util::CString lhs, util::CString rhs);

private:
    char* buffer = nullptr;
    size_t length = 0;
};

} // namespace util

namespace std
{
template <>
struct hash<util::CString>
{
    size_t operator()(util::CString const& str) const noexcept
    {
        return (hash<const char*>()(str.c_str()));
    }
};
} // namespace std
