// VyroEngine — Animation tests
#include "vyro/animation/AnimationClip.hpp"
#include "vyro/animation/Blend.hpp"
#include "vyro/animation/StateMachine.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
bool approx(vyro::Vec3 a, vyro::Vec3 b)
{
    return approx(a.x, b.x) && approx(a.y, b.y) && approx(a.z, b.z);
}
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("animation");

    // AnimationTrack_Sample_InterpolatesBetweenKeys (6.1)
    {
        AnimationClip clip("walk", 2.0f, false);
        AnimationTrack& track = clip.add_track();
        track.add_key(0.0f, Vec3{0, 0, 0});
        track.add_key(2.0f, Vec3{10, 0, 0});
        suite.check(approx(clip.sample(0, 1.0f), Vec3{5, 0, 0}), "midpoint interpolates to half");
        suite.check(approx(clip.sample(0, 0.0f), Vec3{0, 0, 0}), "start key exact");
        suite.check(approx(clip.sample(0, 5.0f), Vec3{10, 0, 0}), "clamps past last key");
    }

    // AnimationClip_Looping_WrapsTime (6.1)
    {
        AnimationClip clip("spin", 2.0f, true);
        AnimationTrack& track = clip.add_track();
        track.add_key(0.0f, Vec3{0, 0, 0});
        track.add_key(2.0f, Vec3{4, 0, 0});
        // t=3 wraps to t=1 -> halfway -> x=2
        suite.check(approx(clip.sample(0, 3.0f), Vec3{2, 0, 0}), "looping wraps sample time");
    }

    // Blend_Poses_LerpsPerBone (6.2)
    {
        Pose a{Vec3{0, 0, 0}, Vec3{10, 0, 0}};
        Pose b{Vec3{0, 10, 0}, Vec3{10, 10, 0}};
        Pose mixed = blend(a, b, 0.5f);
        suite.check(mixed.size() == 2, "blended pose has both bones");
        suite.check(approx(mixed[0], Vec3{0, 5, 0}), "bone 0 blended halfway");
        suite.check(approx(mixed[1], Vec3{10, 5, 0}), "bone 1 blended halfway");
    }

    // StateMachine_Transition_FiresOnParameterThreshold (6.3)
    {
        AnimationStateMachine sm;
        const auto idle = sm.add_state("Idle");
        const auto run = sm.add_state("Run");
        sm.add_transition(idle, run, "speed", 3.0f);
        sm.set_current(idle);

        sm.set_parameter("speed", 1.0f);
        sm.update(0.1f);
        suite.check(sm.current() == idle, "stays Idle below threshold");

        sm.set_parameter("speed", 5.0f);
        sm.update(0.1f);
        suite.check(sm.current() == run, "transitions to Run above threshold");
        suite.check(sm.current_name() == "Run", "current name reflects state");
        suite.check(approx(sm.time(), 0.0f), "local time resets on transition");
    }

    return suite.summary();
}
