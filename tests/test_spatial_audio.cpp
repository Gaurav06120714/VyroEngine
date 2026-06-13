// VyroEngine — Spatial audio tests (V7.4)
#include "vyro/audio/Spatial.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("spatial_audio");

    // Distance attenuation.
    suite.check(approx(audio::attenuation(1.0f, 2.0f, 20.0f), 1.0f), "within min is full volume");
    suite.check(approx(audio::attenuation(20.0f, 2.0f, 20.0f), 0.0f), "at max is silent");
    suite.check(approx(audio::attenuation(11.0f, 2.0f, 20.0f), 0.5f), "midpoint is half volume");
    suite.check(audio::attenuation(100.0f, 2.0f, 20.0f) == 0.0f, "beyond max stays silent");

    // Pan: right axis is +x. A source to the right pans right; left pans left.
    {
        const Vec3 ear{0, 0, 0};
        const Vec3 right{1, 0, 0};
        suite.check(audio::pan(ear, right, Vec3{5, 0, 0}) > 0.9f, "source on the right pans right");
        suite.check(audio::pan(ear, right, Vec3{-5, 0, 0}) < -0.9f, "source on the left pans left");
        suite.check(approx(audio::pan(ear, right, Vec3{0, 0, -5}), 0.0f), "source ahead is centered");
    }

    // Combined gain: a close right-side source is louder in the right ear.
    {
        const auto g = audio::spatial_gain(Vec3{0, 0, 0}, Vec3{1, 0, 0}, Vec3{3, 0, 0}, 2.0f, 20.0f);
        suite.check(g.right > g.left, "right-side source louder on the right");
        suite.check(g.right > 0.0f && g.left >= 0.0f, "gains are non-negative");
    }
    // A distant source is quiet in both ears.
    {
        const auto g =
            audio::spatial_gain(Vec3{0, 0, 0}, Vec3{1, 0, 0}, Vec3{0, 0, -100}, 2.0f, 20.0f);
        suite.check(g.left < 0.01f && g.right < 0.01f, "far source is near-silent");
    }

    return suite.summary();
}
