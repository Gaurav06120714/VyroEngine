// VyroEngine — Collision detection implementation
#include "vyro/physics/Collision.hpp"

#include <algorithm>
#include <cmath>

namespace vyro {

Contact collide(const Sphere& a, const Sphere& b)
{
    Contact c;
    const Vec3 delta = b.center - a.center;
    const f32 dist_sq = dot(delta, delta);
    const f32 radii = a.radius + b.radius;
    if (dist_sq >= radii * radii) {
        return c; // not colliding
    }

    const f32 dist = std::sqrt(dist_sq);
    c.colliding = true;
    c.penetration = radii - dist;
    // Degenerate (concentric) case: pick an arbitrary axis.
    c.normal = dist > 0.0f ? delta * (1.0f / dist) : Vec3{1.0f, 0.0f, 0.0f};
    return c;
}

bool overlaps(const AABB& a, const AABB& b)
{
    return a.min.x <= b.max.x && a.max.x >= b.min.x
        && a.min.y <= b.max.y && a.max.y >= b.min.y
        && a.min.z <= b.max.z && a.max.z >= b.min.z;
}

bool contains(const AABB& box, Vec3 point)
{
    return point.x >= box.min.x && point.x <= box.max.x
        && point.y >= box.min.y && point.y <= box.max.y
        && point.z >= box.min.z && point.z <= box.max.z;
}

Contact collide(const AABB& a, const AABB& b)
{
    Contact c;
    if (!overlaps(a, b)) {
        return c;
    }

    // Overlap extents on each axis; the smallest is the separation axis.
    const f32 ox = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    const f32 oy = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    const f32 oz = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

    const Vec3 a_center{(a.min.x + a.max.x) * 0.5f, (a.min.y + a.max.y) * 0.5f, (a.min.z + a.max.z) * 0.5f};
    const Vec3 b_center{(b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f, (b.min.z + b.max.z) * 0.5f};

    c.colliding = true;
    if (ox <= oy && ox <= oz) {
        c.penetration = ox;
        c.normal = {b_center.x < a_center.x ? -1.0f : 1.0f, 0.0f, 0.0f};
    } else if (oy <= ox && oy <= oz) {
        c.penetration = oy;
        c.normal = {0.0f, b_center.y < a_center.y ? -1.0f : 1.0f, 0.0f};
    } else {
        c.penetration = oz;
        c.normal = {0.0f, 0.0f, b_center.z < a_center.z ? -1.0f : 1.0f};
    }
    return c;
}

} // namespace vyro
