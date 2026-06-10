// VyroEngine — Physics world tests
#include "vyro/physics/PhysicsWorld.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-3f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("physics");

    // PhysicsWorld_Gravity_AcceleratesDynamicBody
    {
        PhysicsWorld world;
        RigidBody b;
        b.position = Vec3{0, 10, 0};
        b.set_mass(1.0f);
        const BodyId id = world.add_body(b);

        world.step(); // one fixed step (1/60 s)
        const RigidBody& after = world.body(id);
        suite.check(after.velocity.y < 0.0f, "gravity pulls velocity down");
        suite.check(approx(after.velocity.y, -9.81f / 60.0f), "velocity = g * dt");
        suite.check(after.position.y < 10.0f, "body fell");
    }

    // PhysicsWorld_StaticBody_DoesNotMove
    {
        PhysicsWorld world;
        RigidBody floor;
        floor.position = Vec3{0, 0, 0};
        floor.set_mass(0.0f); // static
        const BodyId id = world.add_body(floor);
        suite.check(world.body(id).is_static(), "zero mass is static");
        world.step();
        suite.check(world.body(id).position.y == 0.0f, "static body stays put");
    }

    // PhysicsWorld_Collision_SeparatesBodies
    {
        PhysicsWorld world;
        world.set_gravity(Vec3{0, 0, 0}); // isolate collision response

        RigidBody a;
        a.position = Vec3{0, 0, 0};
        a.radius = 1.0f;
        a.velocity = Vec3{1, 0, 0}; // moving toward b
        a.set_mass(1.0f);

        RigidBody b;
        b.position = Vec3{1.5f, 0, 0}; // overlapping (sum radii 2 > 1.5)
        b.radius = 1.0f;
        b.velocity = Vec3{-1, 0, 0}; // moving toward a
        b.set_mass(1.0f);

        const BodyId ia = world.add_body(a);
        const BodyId ib = world.add_body(b);

        world.step();
        suite.check(world.stats().contacts_last_step == 1, "one contact resolved");
        // Equal-mass head-on with restitution should reverse approach velocities.
        suite.check(world.body(ia).velocity.x <= 0.0f, "body A pushed back (-x)");
        suite.check(world.body(ib).velocity.x >= 0.0f, "body B pushed back (+x)");
    }

    // PhysicsWorld_Update_RunsFixedSteps
    {
        PhysicsWorld world;
        world.add_body(RigidBody{});
        world.update(1.0f / 60.0f * 3.0f + 0.001f); // ~3 fixed steps
        suite.check(world.stats().steps == 3, "update runs three fixed steps");
    }

    return suite.summary();
}
