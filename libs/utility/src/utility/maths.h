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

#define M_DBL_PI 6.28318530718
#define M_HALF_PI 1.57079632679
#define M_EPSILON 0.00001

namespace util
{
namespace maths
{
/**
 * @brief Convert degrees to radians.
 *
 * @tparam T The floating point precision to use.
 * @param deg The value to convert in degrees.
 * @return T The value converted to radians.
 */
template <typename T>
T radians(const T deg)
{
    return deg * static_cast<T>(M_PI / 180.0);
}

/// @brief For some reason I can't find the ToRotationMatrix in the latest
/// release for mathfu though it should be there! So had to implement this
/// myself for now: Extracts the 3x3 rotation Matrix from a 4x4 Matrix.
///
/// This resulting Matrix will contain the upper-left 3x3 sub-matrix of the
/// input Matrix.
///
/// @param m 4x4 Matrix.
/// @return rotation Matrix containing the result.
static inline mathfu::mat3 ToRotationMatrix(const mathfu::mat4& m)
{
    return mathfu::mat3 {m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10]};
}

} // namespace maths
} // namespace util
