/* Copyright (c) 2018-2020 Garry Whitehead
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

// forward declarations
struct AABBox;

class Frustum
{
public:
    Frustum() = default;

    void projection(const mathfu::mat4& viewProj);

    bool checkSphereIntersect(const mathfu::vec3& pos, float radius);

    void checkIntersection(
        const mathfu::vec3* __restrict centers,
        const mathfu::vec3* __restrict extents,
        size_t count,
        uint8_t* __restrict results) noexcept;

    bool checkIntersection(const AABBox& box);

private:
    std::array<mathfu::vec4, 6> planes_;
};

} // namespace yave
