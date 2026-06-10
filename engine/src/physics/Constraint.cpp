// VyroEngine — Physics constraints implementation
#include "vyro/physics/Constraint.hpp"

#include "vyro/math/Vec.hpp"

namespace vyro {

void DistanceConstraint::solve(RigidBody& a, RigidBody& b) const
{
    const f32 inv_sum = a.inverse_mass + b.inverse_mass;
    if (inv_sum == 0.0f) {
        return; // both static
    }

    const Vec3 delta = b.position - a.position;
    const f32 dist = length(delta);
    if (dist == 0.0f) {
        return; // coincident; no defined direction
    }

    // Fractional error: positive when stretched, negative when compressed.
    const f32 diff = (dist - rest_length) / dist;

    a.position = a.position + delta * (diff * (a.inverse_mass / inv_sum));
    b.position = b.position - delta * (diff * (b.inverse_mass / inv_sum));
}

} // namespace vyro
