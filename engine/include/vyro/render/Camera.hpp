// VyroEngine — Camera
// Phase 3.5: holds view and projection matrices and produces a combined
// view-projection for the renderer. Supports perspective and orthographic
// projections; the view is built from an eye/target/up basis.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/math/Vec.hpp"

namespace vyro {

enum class ProjectionType : u8 {
    Perspective,
    Orthographic,
};

class Camera
{
public:
    Camera() = default;

    void set_perspective(f32 fov_y_radians, f32 aspect, f32 near_z, f32 far_z);
    void set_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_z, f32 far_z);

    // Build the view matrix from an eye/target/up basis.
    void set_view(Vec3 eye, Vec3 target, Vec3 up = Vec3{0.0f, 1.0f, 0.0f});

    [[nodiscard]] const Mat4& view() const { return m_view; }
    [[nodiscard]] const Mat4& projection() const { return m_projection; }
    [[nodiscard]] Mat4 view_projection() const { return m_projection * m_view; }

    [[nodiscard]] Vec3 position() const { return m_position; }
    [[nodiscard]] ProjectionType projection_type() const { return m_type; }

private:
    Mat4 m_view = Mat4::identity();
    Mat4 m_projection = Mat4::identity();
    Vec3 m_position{};
    ProjectionType m_type = ProjectionType::Perspective;
};

} // namespace vyro
