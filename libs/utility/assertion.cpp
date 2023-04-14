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

#include "assertion.h"

#include <cstdarg>
#include <exception>
#include <string>

namespace util
{
namespace assertion
{

std::string formatArgs(char const* format, va_list args)
{
    std::string error;

    va_list tmp;
    va_copy(tmp, args);

    int n = vsnprintf(nullptr, 0, format, tmp);
    va_end(tmp);

    if (n >= 0)
    {
        ++n;
        char* buf = new char[n];
        vsnprintf(buf, size_t(n), format, args);
        error.assign(buf);
        delete[] buf;
    }
    return error;
}

std::string createErrorMsg(
    const char* function, const char* file, int line, const char* error)
{
#ifndef NDEBUG
    return std::string(
        "\nError at line " + std::to_string(line) + "\nFile: " +
        std::string(file) + "\nFunction: " + std::string(error) + "\n\n");
#else
    return std::string(
        "Error at line " + std::to_string(line) + "\n" + std::string(error));
#endif
}

void fatal(
    char const* function, char const* file, int line, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    std::string error(formatArgs(format, args));
    va_end(args);

    std::string msg = createErrorMsg(function, file, line, error.c_str());

#ifdef UTIL_EXCEPTION_ENABLED
    throw std::runtime_error(msg);
#endif
    std::abort();
}

void log(char const* function, char const* file, int line, const char* str, ...)
{
    va_list args;
    va_start(args, str);
    std::string error(formatArgs(function, args));
    va_end(args);
    std::string msg = createErrorMsg(function, file, line, error.c_str());
    fprintf(stdout, "%s", msg.c_str());
}

} // namespace assertion
} // namespace util
