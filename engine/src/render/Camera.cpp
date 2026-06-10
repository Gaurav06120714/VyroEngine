// VyroEngine — Camera implementation
#include "vyro/render/Camera.hpp"

namespace vyro {

void Camera::set_perspective(f32 fov_y_radians, f32 aspect, f32 near_z, f32 far_z)
{
    m_projection = Mat4::perspective(fov_y_radians, aspect, near_z, far_z);
    m_type = ProjectionType::Perspective;
}

void Camera::set_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_z, f32 far_z)
{
    m_projection = Mat4::orthographic(left, right, bottom, top, near_z, far_z);
    m_type = ProjectionType::Orthographic;
}

void Camera::set_view(Vec3 eye, Vec3 target, Vec3 up)
{
    m_position = eye;
    m_view = Mat4::look_at(eye, target, up);
}

} // namespace vyro
