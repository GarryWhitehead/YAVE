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

#include <cstdint>

namespace yave
{
class Object;

class LightManager
{
public:
    enum class Type
    {
        Spot,
        Point,
        Directional
    };

    struct CreateInfo
    {
        // position of the light in world space
        mathfu::vec3 position = {};
        mathfu::vec3 target = {};

        /// the colour of the light
        mathfu::vec3 colour {1.0f};

        /// the field of view of this light
        float fov = 90.0f;

        /// the light intensity in lumens
        float intensity = 100.0f;

        float fallout = 0.0f;
        float radius = 0.0f;
        float scale = 0.0f;
        float offset = 0.0f;

        // used for deriving the spotlight intensity
        float innerCone = 5.0f;
        float outerCone = 10.0f;
    };

    virtual ~LightManager();

    virtual void create(const CreateInfo& ci, Type type, Object* obj) = 0;

    virtual void prepare() = 0;

    virtual void setIntensity(float intensity, Object* obj) = 0;

    virtual void setFallout(float fallout, Object* obj) = 0;

    virtual void setPosition(const mathfu::vec3& pos, Object* obj) = 0;

    virtual void setTarget(const mathfu::vec3& target, Object* obj) = 0;

    virtual void setColour(const mathfu::vec3& col, Object* obj) = 0;

    virtual void setFov(float fov, Object* obj) = 0;

protected:
    LightManager();
};

} // namespace yave