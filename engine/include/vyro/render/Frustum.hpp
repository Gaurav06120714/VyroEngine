// VyroEngine — View frustum & culling (V5.3)
// Extracts the six clipping planes from a view-projection matrix
// (Gribb-Hartmann) and tests bounding volumes against them, so larger worlds
// can skip drawing whatever the camera can't see. Headless and unit-tested.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/math/Vec.hpp"

#include <array>
#include <cmath>

namespace vyro {

// Inside half-space is where dot(normal, p) + d >= 0.
struct Plane {
    Vec3 normal{};
    f32 d = 0.0f;
};

struct Frustum {
    std::array<Plane, 6> planes{}; // left, right, bottom, top, near, far
};

// Build a frustum from a view-projection matrix (column-major, M*v convention).
[[nodiscard]] inline Frustum frustum_from_view_projection(const Mat4& m)
{
    // Rows of the matrix; clip.w-row ± clip.{x,y,z}-row give the planes.
    auto row = [&](usize r) {
        return std::array<f32, 4>{m.at(r, 0), m.at(r, 1), m.at(r, 2), m.at(r, 3)};
    };
    const auto r0 = row(0);
    const auto r1 = row(1);
    const auto r2 = row(2);
    const auto r3 = row(3);

    auto make = [](const std::array<f32, 4>& a, const std::array<f32, 4>& b, f32 sign) {
        Plane p;
        p.normal = {a[0] + sign * b[0], a[1] + sign * b[1], a[2] + sign * b[2]};
        p.d = a[3] + sign * b[3];
        const f32 len = std::sqrt(p.normal.x * p.normal.x + p.normal.y * p.normal.y
                                  + p.normal.z * p.normal.z);
        if (len > 0.0f) {
            const f32 inv = 1.0f / len;
            p.normal = p.normal * inv;
            p.d *= inv;
        }
        return p;
    };

    Frustum f;
    f.planes[0] = make(r3, r0, +1.0f); // left  : w + x
    f.planes[1] = make(r3, r0, -1.0f); // right : w - x
    f.planes[2] = make(r3, r1, +1.0f); // bottom: w + y
    f.planes[3] = make(r3, r1, -1.0f); // top   : w - y
    f.planes[4] = make(r3, r2, +1.0f); // near  : w + z
    f.planes[5] = make(r3, r2, -1.0f); // far   : w - z
    return f;
}

// True if the sphere is not entirely outside any plane.
[[nodiscard]] inline bool intersects_sphere(const Frustum& f, Vec3 center, f32 radius)
{
    for (const Plane& p : f.planes) {
        if (dot(p.normal, center) + p.d < -radius) {
            return false; // fully behind this plane
        }
    }
    return true;
}

// True if the axis-aligned box [min,max] is not entirely outside any plane.
[[nodiscard]] inline bool intersects_aabb(const Frustum& f, Vec3 min, Vec3 max)
{
    for (const Plane& p : f.planes) {
        // The box corner furthest along the plane normal (positive vertex).
        const Vec3 pv{p.normal.x >= 0.0f ? max.x : min.x, p.normal.y >= 0.0f ? max.y : min.y,
                      p.normal.z >= 0.0f ? max.z : min.z};
        if (dot(p.normal, pv) + p.d < 0.0f) {
            return false; // even the nearest-inside corner is outside
        }
    }
    return true;
}

} // namespace vyro
