// VyroEngine — Physics debug draw
// Phase 4.5: collects debug primitives (collider outlines, contact points)
// for visualization. The renderer consumes these; here they are recorded so
// tooling and tests can inspect the simulation without a GPU.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"
#include "vyro/physics/PhysicsWorld.hpp"

#include <vector>

namespace vyro {

struct DebugLine {
    Vec3 from{};
    Vec3 to{};
};

struct DebugSphere {
    Vec3 center{};
    f32 radius = 0.0f;
};

class PhysicsDebugDraw
{
public:
    void clear();

    void add_line(Vec3 from, Vec3 to);
    void add_sphere(Vec3 center, f32 radius);

    // Record one debug sphere per body in the world.
    void capture(const PhysicsWorld& world);

    [[nodiscard]] const std::vector<DebugLine>& lines() const { return m_lines; }
    [[nodiscard]] const std::vector<DebugSphere>& spheres() const { return m_spheres; }

private:
    std::vector<DebugLine> m_lines;
    std::vector<DebugSphere> m_spheres;
};

} // namespace vyro
