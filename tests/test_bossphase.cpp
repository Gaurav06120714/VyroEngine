// VyroEngine — Boss phase tests (V10.2)
#include "vyro/game/BossPhase.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("bossphase");

    // Phase by health fraction (max 100).
    suite.check(game::boss_phase(100, 100) == game::BossPhase::Calm, "full health is calm");
    suite.check(game::boss_phase(80, 100) == game::BossPhase::Calm, ">66% is calm");
    suite.check(game::boss_phase(50, 100) == game::BossPhase::Enraged, "half is enraged");
    suite.check(game::boss_phase(20, 100) == game::BossPhase::Frenzied, "<33% is frenzied");
    suite.check(game::boss_phase(0, 100) == game::BossPhase::Frenzied, "near death is frenzied");
    suite.check(game::boss_phase(5, 0) == game::BossPhase::Calm, "zero max is safe");

    // Speed escalates with the phase.
    suite.check(game::phase_speed_mult(game::BossPhase::Calm) == 1.0f, "calm is baseline speed");
    suite.check(game::phase_speed_mult(game::BossPhase::Enraged)
                    > game::phase_speed_mult(game::BossPhase::Calm),
                "enraged is faster");
    suite.check(game::phase_speed_mult(game::BossPhase::Frenzied)
                    > game::phase_speed_mult(game::BossPhase::Enraged),
                "frenzied is fastest");

    return suite.summary();
}
