// VyroEngine — Physics debug draw implementation
#include "vyro/physics/PhysicsDebug.hpp"

namespace vyro {

void PhysicsDebugDraw::clear()
{
    m_lines.clear();
    m_spheres.clear();
}

void PhysicsDebugDraw::add_line(Vec3 from, Vec3 to)
{
    m_lines.push_back(DebugLine{from, to});
}

void PhysicsDebugDraw::add_sphere(Vec3 center, f32 radius)
{
    m_spheres.push_back(DebugSphere{center, radius});
}

void PhysicsDebugDraw::capture(const PhysicsWorld& world)
{
    clear();
    for (usize i = 0; i < world.body_count(); ++i) {
        const RigidBody& b = world.body(static_cast<BodyId>(i));
        add_sphere(b.position, b.radius);
    }
}

} // namespace vyro
