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

#include <cstdarg>
#include <cstdio>
#include <string>

struct Logger
{
    static std::string formatArgs(char const* format, va_list args)
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

    static void print(const char* type, int line, const char* file, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        std::string msg(Logger::formatArgs(fmt, args));
        va_end(args);

        fprintf(stderr, "[%s: %d:%s] %s\n", type, line, file, msg.c_str());
        fflush(stderr);
    }
};

// simple file logging tools
#define LOGGER(fmt, ...) Logger::print("", __LINE__, __FILE__, fmt, ##__VA_ARGS__);

#define LOGGER_ERROR(fmt, ...) Logger::print("Error", __LINE__, __FILE__, fmt, ##__VA_ARGS__);

#define LOGGER_INFO(fmt, ...) Logger::print("Info", __LINE__, __FILE__, fmt, ##__VA_ARGS__);

#define LOGGER_WARN(fmt, ...) Logger::print("Warn", __LINE__, __FILE__, fmt, ##__VA_ARGS__);
