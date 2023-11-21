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

#include "utility/maths.h"

#include <mathfu/glsl_mappings.h>

namespace yave
{

struct AABBox
{
    /// 3D extents (min, max) of this box in object space
    mathfu::vec3 min {-1.0f};
    mathfu::vec3 max {1.0f};

    static AABBox calculateRigidTransform(AABBox& box, const mathfu::mat4& world)
    {
        AABBox ret;
        mathfu::mat3 rot = util::maths::ToRotationMatrix(world);
        mathfu::vec3 trans = world.TranslationVector3D();
        ret.min = rot * box.min + trans;
        ret.max = rot * box.max + trans;
        return ret;
    }

    /**
     * Calculates the center position of the box
     */
    [[nodiscard]] mathfu::vec3 getCenter() const { return (max + min) * mathfu::vec3 {0.5f}; }

    /**
     * Calculates the half extent of the box
     */
    [[nodiscard]] mathfu::vec3 getHalfExtent() const { return (max - min) * mathfu::vec3 {0.5f}; }
};

} // namespace yave
