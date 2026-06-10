// VyroEngine — Rigid body
// Phase 4.3: a sphere-shaped dynamic body. inverse_mass == 0 marks a static
// (immovable) body. Restitution controls bounciness in impulse resolution.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"
#include "vyro/physics/Collision.hpp"

namespace vyro {

struct RigidBody {
    Vec3 position{};
    Vec3 velocity{};
    Vec3 force{};

    f32 inverse_mass = 1.0f; // 0 == static
    f32 restitution = 0.5f;
    f32 radius = 0.5f;

    [[nodiscard]] bool is_static() const { return inverse_mass == 0.0f; }

    [[nodiscard]] Sphere shape() const { return Sphere{position, radius}; }

    // Set mass; 0 or negative makes the body static.
    void set_mass(f32 mass) { inverse_mass = mass > 0.0f ? 1.0f / mass : 0.0f; }
    [[nodiscard]] f32 mass() const { return inverse_mass > 0.0f ? 1.0f / inverse_mass : 0.0f; }
};

} // namespace vyro
