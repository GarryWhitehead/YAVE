#include <private/camera.h>

namespace yave
{

void Camera::setProjection(float fovy, float aspect, float near, float far, ProjectionType type)
{
    static_cast<ICamera*>(this)->setProjectionMatrix(fovy, aspect, near, far, type);
}

void Camera::setViewMatrix(const mathfu::mat4& lookAt)
{
    static_cast<ICamera*>(this)->setViewMatrix(lookAt);
}

void Camera::setFov(float fovy)
{
    static_cast<ICamera*>(this)->setFov(fovy);
}

}