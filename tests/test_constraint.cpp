// VyroEngine — Constraint and physics debug tests
#include "vyro/physics/Constraint.hpp"
#include "vyro/physics/PhysicsDebug.hpp"
#include "vyro/physics/PhysicsWorld.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-3f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("constraint");

    // DistanceConstraint_Stretched_PullsBodiesToRestLength
    {
        RigidBody a;
        a.position = Vec3{0, 0, 0};
        a.set_mass(1.0f);
        RigidBody b;
        b.position = Vec3{4, 0, 0}; // 4 apart
        b.set_mass(1.0f);

        DistanceConstraint c{2.0f}; // want them 2 apart
        c.solve(a, b);
        const f32 new_dist = length(b.position - a.position);
        suite.check(approx(new_dist, 2.0f), "equal-mass constraint reaches rest length");
        suite.check(approx(a.position.x, 1.0f) && approx(b.position.x, 3.0f),
                    "both moved symmetrically");
    }

    // DistanceConstraint_StaticAnchor_OnlyDynamicMoves
    {
        RigidBody anchor;
        anchor.position = Vec3{0, 0, 0};
        anchor.set_mass(0.0f); // static
        RigidBody body;
        body.position = Vec3{5, 0, 0};
        body.set_mass(1.0f);

        DistanceConstraint c{2.0f};
        c.solve(anchor, body);
        suite.check(anchor.position.x == 0.0f, "static anchor does not move");
        suite.check(approx(body.position.x, 2.0f), "dynamic body pulled to rest length");
    }

    // PhysicsDebugDraw_Capture_RecordsSpherePerBody
    {
        PhysicsWorld world;
        RigidBody a;
        a.position = Vec3{1, 2, 3};
        a.radius = 0.5f;
        world.add_body(a);
        RigidBody b;
        b.position = Vec3{-1, 0, 0};
        b.radius = 1.0f;
        world.add_body(b);

        PhysicsDebugDraw debug;
        debug.capture(world);
        suite.check(debug.spheres().size() == 2, "one debug sphere per body");
        suite.check(debug.spheres()[0].center == (Vec3{1, 2, 3}), "first sphere at body position");
        suite.check(approx(debug.spheres()[1].radius, 1.0f), "second sphere radius matches body");
    }

    return suite.summary();
}
