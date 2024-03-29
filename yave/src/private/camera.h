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

#include "uniform_buffer.h"
#include "utility/compiler.h"
#include "yave/camera.h"

#include <mathfu/glsl_mappings.h>

namespace yave
{

class ICamera : public Camera
{
public:
    ICamera();
    ~ICamera();

    void shutDown(vkapi::VkDriver& driver) noexcept;

    void setProjectionMatrix(float fovy, float aspect, float near, float far, ProjectionType type);

    void setFov(float fovy) noexcept;


    // ============== getters ==========================

    mathfu::mat4& projMatrix() noexcept { return projection_; }
    mathfu::mat4& viewMatrix();
    mathfu::vec3 position();
    mathfu::mat4& modelMatrix() noexcept { return model_; }

    [[nodiscard]] float getNear() const noexcept { return near_; }
    [[nodiscard]] float getFar() const noexcept { return far_; }

    // =============== setters =========================

    void setViewMatrix(const mathfu::mat4& view);

private:
    // current matrices
    mathfu::mat4 projection_;
    mathfu::mat4 view_;
    mathfu::mat4 model_;

    float fov_;
    float near_;
    float far_;
    float aspect_;
};


} // namespace yave
