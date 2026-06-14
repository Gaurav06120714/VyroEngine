// VyroEngine — Environmental hazards (V10.1)
// Timed damage zones (e.g. fire patches) that hurt whatever stands in them.
// Pure helpers: a horizontal containment test, an expiry check, and a
// fractional-damage accumulator that turns damage-per-second into whole hits.
// Headless and tested.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

#include <cmath>

namespace vyro::game {

struct Hazard {
    Vec3 center{};
    f32 radius = 2.0f;
    f32 life = 5.0f;     // seconds remaining
    f32 dps = 4.0f;      // damage per second to occupants
    f32 carry = 0.0f;    // accumulated fractional damage
};

// Is `p` within the hazard's radius (ignoring height)?
[[nodiscard]] inline bool hazard_contains(const Hazard& h, Vec3 p)
{
    const f32 dx = p.x - h.center.x;
    const f32 dz = p.z - h.center.z;
    return (dx * dx + dz * dz) <= (h.radius * h.radius);
}

[[nodiscard]] inline bool hazard_expired(const Hazard& h) { return h.life <= 0.0f; }

// Accumulate dps*dt into `carry` and return the whole-damage amount to apply
// this frame, keeping the remainder for next time.
[[nodiscard]] inline int accrue_damage(f32 dps, f32 dt, f32& carry)
{
    if (dps <= 0.0f || dt <= 0.0f) {
        return 0;
    }
    carry += dps * dt;
    const int whole = static_cast<int>(std::floor(carry));
    carry -= static_cast<f32>(whole);
    return whole;
}

} // namespace vyro::game
