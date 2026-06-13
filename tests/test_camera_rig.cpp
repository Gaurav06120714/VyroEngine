// VyroEngine — Camera rig tests (V4.5)
// Smoothing eases toward a target without overshoot; screen shake saturates,
// decays to rest, scales with trauma, and is deterministic.
#include "vyro/render/CameraRig.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("camera_rig");

    // smooth_factor stays in [0,1] and grows with dt.
    suite.check(approx(smooth_factor(5.0f, 0.0f), 0.0f), "zero dt yields no movement");
    suite.check(smooth_factor(5.0f, 0.016f) > 0.0f && smooth_factor(5.0f, 0.016f) < 1.0f,
                "a frame step is a partial lerp");
    suite.check(smooth_factor(5.0f, 100.0f) <= 1.0f, "factor never exceeds 1");

    // smooth_follow eases toward the target and stays short of it in one step.
    {
        const Vec3 from{0, 0, 0};
        const Vec3 to{10, 0, 0};
        const Vec3 stepped = smooth_follow(from, to, 5.0f, 0.1f);
        suite.check(stepped.x > 0.0f && stepped.x < 10.0f, "one step moves part-way");
        // Iterating converges close to the target.
        Vec3 c = from;
        for (int i = 0; i < 200; ++i) {
            c = smooth_follow(c, to, 5.0f, 0.05f);
        }
        suite.check(approx(c.x, 10.0f, 1e-2f), "iteration converges to target");
        suite.check(smooth_follow(to, to, 5.0f, 0.1f) == to, "equal positions stay put");
    }

    // Trauma saturates at 1 and the offset is bounded by the requested max.
    {
        ScreenShake shake;
        shake.add_trauma(2.0f);
        suite.check(approx(shake.trauma(), 1.0f), "trauma saturates at 1");
        shake.update(0.0f); // advance tick without decay
        const Vec3 o = shake.offset(0.5f);
        suite.check(std::fabs(o.x) <= 0.5f && std::fabs(o.y) <= 0.5f, "offset within max");
        suite.check(approx(o.z, 0.0f), "shake stays in the screen plane");
    }

    // No trauma means no shake.
    {
        ScreenShake calm;
        calm.update(0.016f);
        const Vec3 o = calm.offset(1.0f);
        suite.check(approx(o.x, 0.0f) && approx(o.y, 0.0f), "no trauma, no offset");
    }

    // Trauma decays to zero over time.
    {
        ScreenShake shake(2.0f);
        shake.add_trauma(1.0f);
        for (int i = 0; i < 60; ++i) {
            shake.update(0.05f); // 3s total at decay 2/s
        }
        suite.check(approx(shake.trauma(), 0.0f), "trauma decays to rest");
    }

    // Bigger trauma shakes harder (trauma-squared response).
    {
        ScreenShake big;
        big.add_trauma(1.0f);
        big.update(0.0f);
        ScreenShake small;
        small.add_trauma(0.3f);
        small.update(0.0f); // same tick -> same noise sample
        const f32 big_mag = std::fabs(big.offset(1.0f).x);
        const f32 small_mag = std::fabs(small.offset(1.0f).x);
        suite.check(big_mag > small_mag, "more trauma, more shake");
    }

    // Deterministic: identical call sequences give identical offsets.
    {
        ScreenShake a;
        ScreenShake b;
        a.add_trauma(0.8f);
        b.add_trauma(0.8f);
        a.update(0.016f);
        b.update(0.016f);
        suite.check(a.offset(1.0f) == b.offset(1.0f), "shake is deterministic");
    }

    return suite.summary();
}
