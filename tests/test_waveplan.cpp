// VyroEngine — Data-driven wave plan tests (V8.5)
#include "vyro/game/GameFlow.hpp"
#include "vyro/game/WavePlan.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-3f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("waveplan");

    // Parse: comments, blanks and malformed lines are skipped.
    {
        const auto plans = game::parse_wave_plans(
            "# level one\n5,2.0\n10,3.5\nbad line\n\n15,1.0\n");
        suite.check(plans.size() == 3, "three valid waves parsed");
        suite.check(plans[0].kills == 5 && approx(plans[0].intermission, 2.0f), "first wave");
        suite.check(plans[2].kills == 15 && approx(plans[2].intermission, 1.0f), "third wave");
    }

    // Empty / all-invalid input yields no plans.
    suite.check(game::parse_wave_plans("nothing valid here\n").empty(), "no valid lines -> empty");

    // GameFlow driven by a parsed plan uses its per-wave targets.
    {
        auto plans = game::parse_wave_plans("2,0.5\n4,0.5\n");
        game::GameFlow flow(std::move(plans));
        suite.check(flow.total_waves() == 2, "two waves from the plan");
        suite.check(flow.required() == 2, "wave 1 target from plan");
        flow.register_kill();
        flow.register_kill(); // clears wave 1
        flow.update(0.5f);    // -> wave 2
        suite.check(flow.wave() == 2 && flow.required() == 4, "wave 2 target from plan");
        flow.register_kill();
        flow.register_kill();
        flow.register_kill();
        flow.register_kill(); // clears final wave
        suite.check(flow.phase() == game::Phase::Victory, "clearing the plan wins");
    }

    return suite.summary();
}
