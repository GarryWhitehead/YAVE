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

#include <string>

namespace util
{

namespace assertion
{
std::string formatArgs(char const* format, va_list args);
std::string createErrorMsg(const char* function, const char* file, int line, const char* error);
void fatal(char const* function, char const* file, int line, const char* format, ...);
void log(char const* function, char const* file, int line, const char* format, ...);
} // namespace assertion

} // namespace util

#ifdef NDEBUG

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define ASSERT_FUNCTION __PRETTY_FUNCTION__
#define ASSERT_FILE(F) (F)

#define ASSERT_LOG(cond)                                                                           \
    (!(cond) ? util::assertion::log(ASSERT_FUNCTION, ASSERT_FILE(__FILE__), __LINE__, #cond)       \
             : (void)0)

#define ASSERT_FATAL(cond, format, ...)                                                            \
    (!(cond) ? util::assertion::fatal(                                                             \
                   ASSERT_FUNCTION, ASSERT_FILE(__FILE__), __LINE__, format, ##__VA_ARGS__)        \
             : (void)0)

#else

#define ASSERT_LOG(cond) ((void)0)
#define ASSERT_FATAL(cond, format, ...) ((void)0)

#endif
