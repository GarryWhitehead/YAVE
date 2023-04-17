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

#include "frustum.h"

#include "aabox.h"

namespace yave
{

void Frustum::projection(const mathfu::mat4& viewProj)
{
    enum Face
    {
        Left,
        Right,
        Top,
        Bottom,
        Back,
        Front
    };

    planes_[Left] = viewProj.GetColumn(3) - viewProj.GetColumn(0);
    planes_[Right] = viewProj.GetColumn(3) + viewProj.GetColumn(0);
    planes_[Top] = viewProj.GetColumn(3) + viewProj.GetColumn(1);
    planes_[Bottom] = viewProj.GetColumn(3) - viewProj.GetColumn(1);
    planes_[Front] = viewProj.GetColumn(3) - viewProj.GetColumn(2);
    planes_[Back] = viewProj.GetColumn(3) + viewProj.GetColumn(2);

    for (uint8_t i = 0; i < 6; ++i)
    {
        float len = mathfu::LengthHelper(planes_[i]);
        planes_[i] /= len;
    }
}

void Frustum::checkIntersection(
    const mathfu::vec3* __restrict centers,
    const mathfu::vec3* __restrict extents,
    size_t count,
    uint8_t* __restrict results) noexcept
{
    for (size_t i = 0; i < 6; ++i)
    {
        for (size_t i = 0; i < count; ++i)
        {
            bool visible = true;

#pragma unroll
            for (size_t j = 0; j < 6; ++j)
            {
                const float dot = planes_[j].x * centers[i].x -
                    std::abs(planes_[j].x) * extents[i].x + planes_[j].y * centers[i].y -
                    std::abs(planes_[j].y) * extents[i].y + planes_[j].z * centers[i].z -
                    std::abs(planes_[j].z) * extents[i].z + planes_[j].w;

                visible &= dot <= 0.0f;
            }
            results[i] = static_cast<uint8_t>(visible);
        }
    }
}

bool Frustum::checkIntersection(const AABBox& box)
{
    uint8_t result = 0;
    const mathfu::vec3 center = box.getCenter();
    const mathfu::vec3 extent = box.getHalfExtent();
    checkIntersection(&center, &extent, 1, &result);
    return static_cast<bool>(result);
}

bool Frustum::checkSphereIntersect(const mathfu::vec3& center, float radius)
{
    for (size_t i = 0; i < 6; ++i)
    {
        const float dot = planes_[i].x * center.x + planes_[i].y * center.y * planes_[i].z +
            center.z + planes_[i].w;
        if (dot <= -radius)
        {
            return false;
        }
    }
    return true;
}

} // namespace yave
