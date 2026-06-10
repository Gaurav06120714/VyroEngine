// VyroEngine — Physics world
// Phase 4.2/4.3/4.5: integrates rigid bodies on a fixed timestep, detects
// sphere-sphere contacts (broad phase O(n^2) for now), and resolves them with
// a sequential-impulse solver plus positional correction. Fixed-step ensures
// deterministic simulation (rulz/PERFORMANCE + SRS).
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"
#include "vyro/physics/RigidBody.hpp"

#include <vector>

namespace vyro {

using BodyId = u32;

class PhysicsWorld : NonCopyable
{
public:
    static constexpr f32 kFixedStep = 1.0f / 60.0f;

    struct Stats {
        u32 contacts_last_step = 0; // contacts resolved in the last step
        u32 steps = 0;              // total fixed steps executed
    };

    PhysicsWorld() = default;

    BodyId add_body(const RigidBody& body);
    [[nodiscard]] RigidBody& body(BodyId id) { return m_bodies[id]; }
    [[nodiscard]] const RigidBody& body(BodyId id) const { return m_bodies[id]; }
    [[nodiscard]] usize body_count() const { return m_bodies.size(); }

    void set_gravity(Vec3 gravity) { m_gravity = gravity; }
    [[nodiscard]] Vec3 gravity() const { return m_gravity; }

    // Advance one fixed step: integrate, detect, resolve.
    void step();

    // Accumulate real delta time and run as many fixed steps as needed.
    void update(f32 dt);

    [[nodiscard]] const Stats& stats() const { return m_stats; }

private:
    void integrate(f32 dt);
    void resolve_contacts();

    std::vector<RigidBody> m_bodies;
    Vec3 m_gravity{0.0f, -9.81f, 0.0f};
    f32 m_accumulator = 0.0f;
    Stats m_stats;
};

} // namespace vyro
