// VyroEngine — Collision detection
// Phase 4.1: primitive shapes and narrow-phase intersection tests that produce
// a contact manifold (normal + penetration depth) consumed by resolution.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

namespace vyro {

struct Sphere {
    Vec3 center{};
    f32 radius = 0.5f;
};

struct AABB {
    Vec3 min{};
    Vec3 max{};
};

// Result of a narrow-phase test. `normal` points from shape A toward shape B.
struct Contact {
    bool colliding = false;
    Vec3 normal{};       // unit separation axis (A -> B)
    f32 penetration = 0.0f; // overlap depth along the normal
};

// Sphere vs sphere.
[[nodiscard]] Contact collide(const Sphere& a, const Sphere& b);

// Axis-aligned box vs box (minimum translation axis).
[[nodiscard]] Contact collide(const AABB& a, const AABB& b);

// Boolean overlap helpers.
[[nodiscard]] bool overlaps(const AABB& a, const AABB& b);
[[nodiscard]] bool contains(const AABB& box, Vec3 point);

} // namespace vyro
