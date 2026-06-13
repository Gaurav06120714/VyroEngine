// VyroEngine — Animation cycle variation tests (V7.1)
// Phase buckets are stable and spread across the clip so the horde doesn't
// march in lockstep.
#include "vyro/animation/CycleVariation.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("cycle_variation");

    // Buckets are in range, stable, and distributed.
    suite.check(anim::phase_bucket(0, 6) == 0 && anim::phase_bucket(7, 6) == 1,
                "bucket is id modulo count");
    suite.check(anim::phase_bucket(42, 6) == anim::phase_bucket(42, 6), "bucket is stable for an id");
    suite.check(anim::phase_bucket(5, 0) == 0, "zero buckets is safe");
    {
        bool all_in_range = true;
        for (u32 id = 0; id < 100; ++id) {
            if (anim::phase_bucket(id, 6) >= 6) {
                all_in_range = false;
            }
        }
        suite.check(all_in_range, "buckets stay in [0, count)");
    }

    // Bucket times span the clip and are evenly spaced.
    {
        const f32 dur = 1.2f;
        suite.check(approx(anim::bucket_time(0, 4, dur), 0.0f), "bucket 0 starts at 0");
        suite.check(approx(anim::bucket_time(1, 4, dur), 0.3f), "bucket 1 is a quarter in");
        suite.check(approx(anim::bucket_time(2, 4, dur), 0.6f), "bucket 2 is halfway");
        suite.check(anim::bucket_time(0, 4, dur) != anim::bucket_time(1, 4, dur),
                    "adjacent buckets differ");
    }

    // Base time shifts all buckets and wraps within the duration.
    {
        const f32 dur = 1.0f;
        const f32 t = anim::bucket_time(2, 4, dur, /*base*/ 0.8f); // 0.8 + 0.5 = 1.3 -> 0.3
        suite.check(approx(t, 0.3f) && t < dur, "base time wraps into the clip");
    }

    return suite.summary();
}
