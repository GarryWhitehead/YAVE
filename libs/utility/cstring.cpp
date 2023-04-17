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

#include "cstring.h"

#include <cassert>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace util
{

CString::CString(const char* str)
{
    if (str)
    {
        length = std::strlen(str);
        if (length > 0)
        {
            buffer = (char*)malloc(length + 1);
            assert(buffer);
            memcpy((char*)buffer, str, length);
            buffer[length] = '\0';
        }
    }
}

CString::CString(const CString& rhs) : CString(rhs.buffer) {}

CString& CString::operator=(const CString& rhs)
{
    if (this != &rhs)
    {
        if (buffer)
        {
            free(buffer);
        }
        new (this) CString(rhs.buffer);
    }
    return *this;
}

bool CString::operator==(const CString& rhs) const noexcept { return rhs.compare(*this); }

bool CString::operator!=(const CString& rhs) const noexcept { return !rhs.compare(*this); }

CString::CString(CString&& rhs) noexcept
    : buffer(std::exchange(rhs.buffer, nullptr)), length(std::exchange(rhs.length, 0))
{
}

CString& CString::operator=(CString&& rhs) noexcept
{
    if (this != &rhs)
    {
        buffer = std::exchange(rhs.buffer, nullptr);
        length = std::exchange(rhs.length, 0);
    }
    return *this;
}

CString::~CString()
{
    if (buffer)
    {
        free(buffer);
        buffer = nullptr;
    }
}

bool CString::compare(CString str) const
{
    assert(buffer);
    assert(str.buffer);

    int diff = std::strcmp(buffer, str.buffer);
    return diff == 0;
}

float CString::toFloat() const { return std::atof(buffer); }

uint32_t CString::toUInt32() const { return std::atoi(buffer); }

uint64_t CString::toUInt64() const { return std::atoll(buffer); }

int CString::toInt() const { return std::atoi(buffer); }

// =============== static functions =============================
std::vector<CString> CString::split(CString str, char identifier)
{
    if (str.empty())
    {
        return {};
    }

    std::vector<CString> result;

    char* bufferPtr = str.buffer;
    std::vector<char> splitBuffer;
    splitBuffer.reserve(str.length);

    for (size_t i = 0; i < str.length; ++i)
    {
        if (*bufferPtr == identifier)
        {
            if (!splitBuffer.empty())
            {
                splitBuffer.emplace_back('\0');
                CString splitStr((const char*)splitBuffer.data());
                result.emplace_back(splitStr);
                splitBuffer.clear();
                ++bufferPtr;
            }
            else
            {
                ++bufferPtr;
                continue;
            }
        }
        splitBuffer.emplace_back(*bufferPtr++);
    }

    if (!splitBuffer.empty())
    {
        splitBuffer.emplace_back('\0');
        CString splitStr((const char*)splitBuffer.data());
        result.emplace_back(splitStr);
    }

    return result;
}

util::CString CString::append(util::CString lhs, util::CString rhs)
{
    if (!lhs.buffer)
    {
        return rhs;
    }
    if (rhs.empty())
    {
        return lhs;
    }

    util::CString newStr;
    size_t newLength = rhs.length + lhs.length;

    char* newBuffer = new char[newLength];
    memcpy(newBuffer, lhs.buffer, lhs.length);
    memcpy(newBuffer + lhs.length, rhs.buffer, rhs.length);
    newBuffer[newLength] = '\0';
    return CString {newBuffer};
}

} // namespace util
