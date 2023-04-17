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

#include "camera_view.h"

#include "utility/maths.h"

namespace yave
{

CameraView::CameraView() : rotation_ {0.0f, 0.0f, 0.0f}, mouseButtonDown_(false), moveSpeed_(0.2f)
{
}

CameraView::~CameraView() {}

void CameraView::keyUpEvent(Movement movement) { keyEvent_[movement] = false; }

void CameraView::keyDownEvent(Movement movement) { keyEvent_[movement] = true; }

void CameraView::mouseButtonDown(double x, double y)
{
    mousePosition_ = {static_cast<float>(x), static_cast<float>(y)};
    mouseButtonDown_ = true;
}

void CameraView::mouseUpdate(double x, double y)
{
    if (!mouseButtonDown_)
    {
        return;
    }

    float dx = x - mousePosition_.x;
    float dy = mousePosition_.y - y;
    mousePosition_.x = x;
    mousePosition_.y = y;

    const float minPitch = (-M_PI_2 + 0.001);
    const float maxPitch = (M_PI_2 - 0.001);
    float pitch = std::clamp(dy * -moveSpeed_, minPitch, maxPitch);
    float yaw = dx * moveSpeed_;

    rotation_.y += pitch;
    rotation_.x += yaw;

    updateView();
}

void CameraView::mouseButtonUp() { mouseButtonDown_ = false; }

void CameraView::updateView()
{
    view_ = mathfu::mat4::LookAt(eye_ + frontVec(), eye_, {0.0f, 1.0f, 0.0f}, 1.0f);
}

mathfu::vec3 CameraView::frontVec() const
{
    return {
        -cos(util::maths::radians(rotation_.y)) * sin(util::maths::radians(rotation_.x)),
        sin(util::maths::radians(rotation_.y)),
        cos(util::maths::radians(rotation_.y)) * cos(util::maths::radians(rotation_.x))};
}

mathfu::vec3 CameraView::rightVec() const
{
    return mathfu::NormalizedHelper(
        mathfu::vec3::CrossProduct(frontVec(), mathfu::vec3(0.0f, 1.0f, 0.0f)));
}

void CameraView::updateKeyEvents(float dt)
{
    float speed = moveSpeed_ * dt;

    if (keyEvent_[Movement::Forward])
    {
        eye_ += frontVec() * speed;
    }
    if (keyEvent_[Movement::Backward])
    {
        eye_ -= frontVec() * speed;
    }
    if (keyEvent_[Movement::Left])
    {
        eye_ -= rightVec() * speed;
    }
    if (keyEvent_[Movement::Right])
    {
        eye_ += rightVec() * speed;
    }

    updateView();
}

void CameraView::setPosition(const mathfu::vec3& pos) { eye_ = pos; }

mathfu::mat4 CameraView::getLookAt() const noexcept { return view_; }

} // namespace yave