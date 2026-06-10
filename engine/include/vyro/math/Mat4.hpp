// VyroEngine — 4x4 matrix math
// Phase 3 (math prereq): column-major 4x4 matrix (OpenGL/GLM convention,
// element (row r, col c) = data[c*4 + r]). Right-handed transforms.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

#include <array>

namespace vyro {

struct Mat4 {
    std::array<f32, 16> data{};

    [[nodiscard]] f32& at(usize row, usize col) { return data[col * 4 + row]; }
    [[nodiscard]] f32 at(usize row, usize col) const { return data[col * 4 + row]; }

    [[nodiscard]] static Mat4 identity();

    // Affine builders.
    [[nodiscard]] static Mat4 translation(Vec3 t);
    [[nodiscard]] static Mat4 scale(Vec3 s);
    [[nodiscard]] static Mat4 rotation(Vec3 axis, f32 radians);

    // Camera / projection builders.
    [[nodiscard]] static Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up);
    [[nodiscard]] static Mat4 perspective(f32 fov_y_radians, f32 aspect, f32 near_z, f32 far_z);
    [[nodiscard]] static Mat4 orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_z, f32 far_z);

    friend bool operator==(const Mat4& a, const Mat4& b) { return a.data == b.data; }
};

// Matrix product (this * rhs).
[[nodiscard]] Mat4 operator*(const Mat4& a, const Mat4& b);

// Transform a column vector / point.
[[nodiscard]] Vec4 operator*(const Mat4& m, Vec4 v);
[[nodiscard]] Vec3 transform_point(const Mat4& m, Vec3 p);

} // namespace vyro
