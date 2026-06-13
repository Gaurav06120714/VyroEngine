// VyroEngine — Game flow tests (V7.5)
// Waves clear on a kill target, intermission gates the next wave, the final
// wave wins, and death loses.
#include "vyro/game/GameFlow.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("gameflow");

    // 3 waves, 2 base kills, +1 per wave, 2s intermission.
    {
        game::GameFlow flow(3, 2, 1, 2.0f);
        suite.check(flow.phase() == game::Phase::Fighting, "starts fighting");
        suite.check(flow.wave() == 1 && flow.required() == 2, "wave 1 needs 2 kills");

        flow.register_kill();
        suite.check(flow.phase() == game::Phase::Fighting, "one kill, still fighting");
        flow.register_kill();
        suite.check(flow.phase() == game::Phase::Intermission, "target met -> intermission");

        flow.update(1.0f);
        suite.check(flow.phase() == game::Phase::Intermission, "still in intermission");
        flow.update(1.5f);
        suite.check(flow.phase() == game::Phase::Fighting && flow.wave() == 2,
                    "intermission ends -> wave 2");
        suite.check(flow.required() == 3, "wave 2 needs 3 kills");
    }

    // Clearing the final wave is victory.
    {
        game::GameFlow flow(2, 1, 0, 1.0f);
        flow.register_kill();            // clears wave 1
        flow.update(1.0f);               // -> wave 2
        suite.check(flow.wave() == 2, "advanced to final wave");
        flow.register_kill();            // clears wave 2 (final)
        suite.check(flow.phase() == game::Phase::Victory, "final wave cleared -> victory");
        suite.check(flow.over(), "victory is game over");
    }

    // Death is defeat, and kills no longer count.
    {
        game::GameFlow flow(3, 5, 0, 1.0f);
        flow.player_died();
        suite.check(flow.phase() == game::Phase::Defeat, "death -> defeat");
        flow.register_kill();
        suite.check(flow.phase() == game::Phase::Defeat, "no progress after defeat");
        suite.check(flow.over(), "defeat is game over");
    }

    // Reset restores wave 1 fighting.
    {
        game::GameFlow flow(3, 2, 1, 2.0f);
        flow.player_died();
        flow.reset();
        suite.check(flow.phase() == game::Phase::Fighting && flow.wave() == 1, "reset restarts");
    }

    return suite.summary();
}
