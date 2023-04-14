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

#include "camera.h"

#include "utility/maths.h"
#include "vulkan-api/pipeline_cache.h"

#include "backend/enums.h"

#include <algorithm>

namespace yave
{

ICamera::ICamera()
    : projection_(mathfu::mat4::Identity()),
      view_(mathfu::mat4::Identity()),
      fov_(0.0f),
      near_(0.0f),
      far_(0.0f),
      aspect_(0.0f)
{
    ubo_ = std::make_unique<UniformBuffer>(
        vkapi::PipelineCache::UboSetValue, 3, "CameraUbo", "camera_ubo");
}

ICamera::~ICamera() {}

void ICamera::shutDown(vkapi::VkDriver& driver) noexcept 
{

}

void ICamera::setProjectionMatrixI(
    float fovy, float aspect, float near, float far, ProjectionType type)
{
    if (type == ProjectionType::Perspective)
    {
        projection_ = mathfu::mat4::Perspective(
            util::maths::radians(fovy), aspect, near, far);
    }
    else
    {
        // TODO: add ortho 
    }

    aspect_ = aspect;
    fov_ = fovy;
    near_ = near;
    far_ = far;

    // flip the y axis for vulkan
    projection_(1, 1) *= -1.0f;
}

mathfu::mat4& ICamera::viewMatrix() 
{ 
    return view_; 
}

void ICamera::setViewMatrix(mathfu::mat4& view) 
{ 
    view_ = view; 
}

void ICamera::setFovI(float fovy) noexcept 
{
    setProjectionMatrixI(
        fovy, aspect_, near_, far_, ProjectionType::Perspective);
}

mathfu::vec3 ICamera::position() 
{ 
    return -view_.TranslationVector3D(); 
}

size_t ICamera::createUbo(vkapi::VkDriver& driver) noexcept
{
    ubo_->pushElement(
        "mvp", backend::BufferElementType::Mat4, sizeof(mathfu::mat4));
    ubo_->pushElement(
        "project",
        backend::BufferElementType::Mat4,
        sizeof(mathfu::mat4));
    ubo_->pushElement(
        "model",
        backend::BufferElementType::Mat4,
        sizeof(mathfu::mat4));
    ubo_->pushElement(
        "view",
        backend::BufferElementType::Mat4,
        sizeof(mathfu::mat4));
    ubo_->pushElement(
        "position",
        backend::BufferElementType::Float3,
        sizeof(mathfu::vec3));
    ubo_->pushElement(
        "padding", backend::BufferElementType::Float, sizeof(float));
    ubo_->pushElement(
        "zNear", backend::BufferElementType::Float, sizeof(float));
    ubo_->pushElement(
        "zFar", backend::BufferElementType::Float, sizeof(float));
    ubo_->createGpuBuffer(driver);
    return ubo_->size();
}

void* ICamera::updateUbo() noexcept
{
    mathfu::mat4 vp = projection_ * view_;
    mathfu::vec3 pos = position();
    ubo_->updateElement("mvp", &vp);
    ubo_->updateElement("project", &projection_);
    ubo_->updateElement("view", &view_);
    ubo_->updateElement("position", &pos);
    ubo_->updateElement("zNear", &near_);
    ubo_->updateElement("zFar", &far_);
    return ubo_->getBlockData();
}

UniformBuffer& ICamera::getUbo() noexcept { return *ubo_; }

// ========================== client api =========================

Camera::Camera() = default;
Camera::~Camera() = default;

void ICamera::setProjection(
    float fovy,
    float aspect,
    float near,
    float far,
    ProjectionType type)
{
    setProjectionMatrixI(fovy, aspect, near, far, type);
}

void ICamera::setViewMatrix(mathfu::mat4 lookAt) 
{ 
    view_ = lookAt; 
}

void ICamera::setFov(float fovy) 
{ 
    setFovI(fovy); 
}

} // namespace yave
