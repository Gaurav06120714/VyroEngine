// VyroEngine — Collision detection tests
#include "vyro/physics/Collision.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("collision");

    // Sphere_Overlapping_ProducesContact
    {
        const Sphere a{Vec3{0, 0, 0}, 1.0f};
        const Sphere b{Vec3{1.5f, 0, 0}, 1.0f}; // centers 1.5 apart, radii sum 2
        const Contact c = collide(a, b);
        suite.check(c.colliding, "overlapping spheres collide");
        suite.check(approx(c.penetration, 0.5f), "penetration = 2 - 1.5");
        suite.check(approx(c.normal.x, 1.0f), "normal points A->B (+x)");
    }

    // Sphere_Separated_NoContact
    {
        const Sphere a{Vec3{0, 0, 0}, 1.0f};
        const Sphere b{Vec3{5, 0, 0}, 1.0f};
        suite.check(!collide(a, b).colliding, "separated spheres do not collide");
    }

    // AABB_Overlap_DetectsAndPicksMinAxis
    {
        const AABB a{Vec3{0, 0, 0}, Vec3{2, 2, 2}};
        const AABB b{Vec3{1.5f, 0, 0}, Vec3{3.5f, 2, 2}}; // overlap 0.5 on x
        suite.check(overlaps(a, b), "boxes overlap");
        const Contact c = collide(a, b);
        suite.check(c.colliding, "AABB contact reported");
        suite.check(approx(c.penetration, 0.5f), "min penetration on x = 0.5");
        suite.check(approx(c.normal.x, 1.0f), "separation normal along +x");
    }

    // AABB_Contains_Point
    {
        const AABB box{Vec3{0, 0, 0}, Vec3{1, 1, 1}};
        suite.check(contains(box, Vec3{0.5f, 0.5f, 0.5f}), "point inside box");
        suite.check(!contains(box, Vec3{2, 0, 0}), "point outside box");
    }

    return suite.summary();
}
