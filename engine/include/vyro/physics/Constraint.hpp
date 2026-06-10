// VyroEngine — Physics constraints
// Phase 4.4: a distance constraint keeps two bodies a fixed distance apart
// (a rigid rod / pin joint). Solved by mass-weighted positional correction,
// layered on top of the same body data the impulse solver uses.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/physics/RigidBody.hpp"

namespace vyro {

struct DistanceConstraint {
    f32 rest_length = 1.0f;

    // Move bodies along their connecting line to satisfy the rest length.
    // Movement is distributed by inverse mass (static bodies do not move).
    void solve(RigidBody& a, RigidBody& b) const;
};

} // namespace vyro
