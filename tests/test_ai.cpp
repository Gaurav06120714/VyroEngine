// VyroEngine — Enemy AI / steering tests (V5.4)
// Seek heads at the target, separation pushes off close neighbors, the horde
// velocity stays within speed, and the behavior state follows distance.
#include "vyro/ai/Steering.hpp"

#include "test_harness.hpp"

#include <cmath>
#include <vector>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-3f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("ai");

    // Behavior state follows distance bands.
    suite.check(ai::select_state(0.5f, 8.0f, 1.5f) == ai::ZombieState::Attack, "close -> attack");
    suite.check(ai::select_state(4.0f, 8.0f, 1.5f) == ai::ZombieState::Seek, "mid -> seek");
    suite.check(ai::select_state(20.0f, 8.0f, 1.5f) == ai::ZombieState::Idle, "far -> idle");

    // Seek points at the target at max speed.
    {
        const Vec3 v = ai::seek(Vec3{0, 0, 0}, Vec3{10, 0, 0}, 2.0f);
        suite.check(approx(v.x, 2.0f) && approx(v.y, 0.0f) && approx(v.z, 0.0f),
                    "seek heads at target at max speed");
        suite.check(approx(length(v), 2.0f), "seek magnitude is max speed");
    }

    // Separation pushes away from a neighbor on the +x side.
    {
        const std::vector<Vec3> neighbors{Vec3{1, 0, 0}};
        const Vec3 push = ai::separation(Vec3{0, 0, 0}, neighbors, 3.0f, 1.0f);
        suite.check(push.x < 0.0f, "separation pushes away from a close neighbor");
    }

    // A neighbor beyond the radius exerts no push.
    {
        const std::vector<Vec3> neighbors{Vec3{50, 0, 0}};
        const Vec3 push = ai::separation(Vec3{0, 0, 0}, neighbors, 3.0f, 1.0f);
        suite.check(approx(length(push), 0.0f), "distant neighbor is ignored");
    }

    // Combined horde velocity never exceeds max speed.
    {
        const std::vector<Vec3> neighbors{Vec3{0.3f, 0, 0}, Vec3{-0.3f, 0, 0}};
        const Vec3 v = ai::horde_velocity(Vec3{0, 0, 0}, Vec3{100, 0, 0}, neighbors, 2.0f, 3.0f,
                                         5.0f);
        suite.check(length(v) <= 2.0f + 1e-3f, "horde velocity clamped to max speed");
    }

    return suite.summary();
}
