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

namespace util
{

struct Colour4
{
public:
    Colour4() = default;
    Colour4(float r, float g, float b, float a) : data_ {r, g, b, a} {}
    Colour4(float r, float g, float b) : data_ {r, g, b, 1.0f} {}
    explicit Colour4(float col) : data_ {col, col, col, 1.0f} {}

    [[nodiscard]] float r() const noexcept { return data_[0]; }
    [[nodiscard]] float g() const noexcept { return data_[1]; }
    [[nodiscard]] float b() const noexcept { return data_[2]; }
    [[nodiscard]] float a() const noexcept { return data_[3]; }

    float r() noexcept { return data_[0]; }
    float g() noexcept { return data_[1]; }
    float b() noexcept { return data_[2]; }
    float a() noexcept { return data_[3]; }

    [[nodiscard]] const float* getData() const noexcept { return data_; }
    float* getData() noexcept { return data_; }

private:
    float data_[4];
};

} // namespace util
