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

#include <mathfu/glsl_mappings.h>

#include <array>

namespace yave
{

struct CubeMap
{
    // cube vertices
    static constexpr std::array<float, 24> Vertices {-1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,  1.0f,  1.0f,
                                          1.0f,  -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f,
                                          -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,  -1.0f};

    // cube indices
    // clang-format off
    static constexpr std::array<uint32_t, 36> Indices {
        0, 1, 2, 2, 3, 0,       // front
        1, 5, 6, 6, 2, 1,       // right side
        7, 6, 5, 5, 4, 7,       // left side
        4, 0, 3, 3, 7, 4,       // bottom
        4, 5, 1, 1, 0, 4,       // back
        3, 2, 6, 6, 7, 3        // top
    };
    // clang-format on

    static mathfu::mat4* createFaceViews() noexcept;
};

} // namespace yave