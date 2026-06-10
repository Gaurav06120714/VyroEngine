// VyroEngine — Physics world implementation
#include "vyro/physics/PhysicsWorld.hpp"

#include "vyro/physics/Collision.hpp"

namespace vyro {

BodyId PhysicsWorld::add_body(const RigidBody& body)
{
    m_bodies.push_back(body);
    return static_cast<BodyId>(m_bodies.size() - 1);
}

void PhysicsWorld::integrate(f32 dt)
{
    for (RigidBody& b : m_bodies) {
        if (b.is_static()) {
            continue;
        }
        // Semi-implicit Euler: apply gravity and accumulated force, then move.
        b.velocity = b.velocity + (m_gravity + b.force * b.inverse_mass) * dt;
        b.position = b.position + b.velocity * dt;
        b.force = Vec3{};
    }
}

void PhysicsWorld::resolve_contacts()
{
    constexpr f32 kCorrectionPercent = 0.8f; // positional correction factor
    constexpr f32 kSlop = 0.001f;            // penetration allowance

    m_stats.contacts_last_step = 0;

    for (usize i = 0; i < m_bodies.size(); ++i) {
        for (usize j = i + 1; j < m_bodies.size(); ++j) {
            RigidBody& a = m_bodies[i];
            RigidBody& b = m_bodies[j];

            const f32 inv_sum = a.inverse_mass + b.inverse_mass;
            if (inv_sum == 0.0f) {
                continue; // both static
            }

            const Contact c = collide(a.shape(), b.shape());
            if (!c.colliding) {
                continue;
            }
            ++m_stats.contacts_last_step;

            // Sequential impulse along the contact normal.
            const Vec3 relative = b.velocity - a.velocity;
            const f32 vel_along_normal = dot(relative, c.normal);
            if (vel_along_normal < 0.0f) {
                const f32 e = a.restitution < b.restitution ? a.restitution : b.restitution;
                const f32 jmag = -(1.0f + e) * vel_along_normal / inv_sum;
                const Vec3 impulse = c.normal * jmag;
                a.velocity = a.velocity - impulse * a.inverse_mass;
                b.velocity = b.velocity + impulse * b.inverse_mass;
            }

            // Positional correction to counter sinking.
            const f32 corr_mag =
                (c.penetration - kSlop > 0.0f ? c.penetration - kSlop : 0.0f) / inv_sum * kCorrectionPercent;
            const Vec3 correction = c.normal * corr_mag;
            a.position = a.position - correction * a.inverse_mass;
            b.position = b.position + correction * b.inverse_mass;
        }
    }
}

void PhysicsWorld::step()
{
    integrate(kFixedStep);
    resolve_contacts();
    ++m_stats.steps;
}

void PhysicsWorld::update(f32 dt)
{
    m_accumulator += dt;
    while (m_accumulator >= kFixedStep) {
        step();
        m_accumulator -= kFixedStep;
    }
}

} // namespace vyro
