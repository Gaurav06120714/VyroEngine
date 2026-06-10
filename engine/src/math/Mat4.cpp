// VyroEngine — 4x4 matrix math implementation
#include "vyro/math/Mat4.hpp"

#include <cmath>

namespace vyro {

Mat4 Mat4::identity()
{
    Mat4 m;
    m.at(0, 0) = 1.0f;
    m.at(1, 1) = 1.0f;
    m.at(2, 2) = 1.0f;
    m.at(3, 3) = 1.0f;
    return m;
}

Mat4 Mat4::translation(Vec3 t)
{
    Mat4 m = identity();
    m.at(0, 3) = t.x;
    m.at(1, 3) = t.y;
    m.at(2, 3) = t.z;
    return m;
}

Mat4 Mat4::scale(Vec3 s)
{
    Mat4 m;
    m.at(0, 0) = s.x;
    m.at(1, 1) = s.y;
    m.at(2, 2) = s.z;
    m.at(3, 3) = 1.0f;
    return m;
}

Mat4 Mat4::rotation(Vec3 axis, f32 radians)
{
    const Vec3 a = normalize(axis);
    const f32 c = std::cos(radians);
    const f32 s = std::sin(radians);
    const f32 t = 1.0f - c;

    Mat4 m = identity();
    m.at(0, 0) = c + a.x * a.x * t;
    m.at(1, 0) = a.y * a.x * t + a.z * s;
    m.at(2, 0) = a.z * a.x * t - a.y * s;

    m.at(0, 1) = a.x * a.y * t - a.z * s;
    m.at(1, 1) = c + a.y * a.y * t;
    m.at(2, 1) = a.z * a.y * t + a.x * s;

    m.at(0, 2) = a.x * a.z * t + a.y * s;
    m.at(1, 2) = a.y * a.z * t - a.x * s;
    m.at(2, 2) = c + a.z * a.z * t;
    return m;
}

Mat4 Mat4::look_at(Vec3 eye, Vec3 target, Vec3 up)
{
    // Right-handed view matrix.
    const Vec3 f = normalize(target - eye); // forward
    const Vec3 r = normalize(cross(f, up)); // right
    const Vec3 u = cross(r, f);             // true up

    Mat4 m = identity();
    m.at(0, 0) = r.x;
    m.at(0, 1) = r.y;
    m.at(0, 2) = r.z;
    m.at(1, 0) = u.x;
    m.at(1, 1) = u.y;
    m.at(1, 2) = u.z;
    m.at(2, 0) = -f.x;
    m.at(2, 1) = -f.y;
    m.at(2, 2) = -f.z;
    m.at(0, 3) = -dot(r, eye);
    m.at(1, 3) = -dot(u, eye);
    m.at(2, 3) = dot(f, eye);
    return m;
}

Mat4 Mat4::perspective(f32 fov_y_radians, f32 aspect, f32 near_z, f32 far_z)
{
    // Right-handed, depth range [0, 1] (Vulkan-style).
    const f32 tan_half = std::tan(fov_y_radians * 0.5f);
    Mat4 m; // all zero
    m.at(0, 0) = 1.0f / (aspect * tan_half);
    m.at(1, 1) = 1.0f / tan_half;
    m.at(2, 2) = far_z / (near_z - far_z);
    m.at(3, 2) = -1.0f;
    m.at(2, 3) = (far_z * near_z) / (near_z - far_z);
    return m;
}

Mat4 Mat4::orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_z, f32 far_z)
{
    Mat4 m = identity();
    m.at(0, 0) = 2.0f / (right - left);
    m.at(1, 1) = 2.0f / (top - bottom);
    m.at(2, 2) = -1.0f / (far_z - near_z);
    m.at(0, 3) = -(right + left) / (right - left);
    m.at(1, 3) = -(top + bottom) / (top - bottom);
    m.at(2, 3) = -near_z / (far_z - near_z);
    return m;
}

Mat4 operator*(const Mat4& a, const Mat4& b)
{
    Mat4 out; // zero
    for (usize col = 0; col < 4; ++col) {
        for (usize row = 0; row < 4; ++row) {
            f32 sum = 0.0f;
            for (usize k = 0; k < 4; ++k) {
                sum += a.at(row, k) * b.at(k, col);
            }
            out.at(row, col) = sum;
        }
    }
    return out;
}

Vec4 operator*(const Mat4& m, Vec4 v)
{
    return {
        m.at(0, 0) * v.x + m.at(0, 1) * v.y + m.at(0, 2) * v.z + m.at(0, 3) * v.w,
        m.at(1, 0) * v.x + m.at(1, 1) * v.y + m.at(1, 2) * v.z + m.at(1, 3) * v.w,
        m.at(2, 0) * v.x + m.at(2, 1) * v.y + m.at(2, 2) * v.z + m.at(2, 3) * v.w,
        m.at(3, 0) * v.x + m.at(3, 1) * v.y + m.at(3, 2) * v.z + m.at(3, 3) * v.w,
    };
}

Vec3 transform_point(const Mat4& m, Vec3 p)
{
    const Vec4 r = m * Vec4{p.x, p.y, p.z, 1.0f};
    return {r.x, r.y, r.z};
}

} // namespace vyro
