// VyroEngine — Medals tests (V9.5)
#include "vyro/game/Medals.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("medals");

    // A plain run earns nothing.
    {
        game::RunResult r;
        r.accuracy = 0.5f;
        r.max_multiplier = 2;
        r.kills = 10;
        suite.check(game::evaluate_medals(r) == 0, "mediocre run earns no medals");
    }

    // Each condition awards its medal independently.
    {
        game::RunResult r;
        r.accuracy = 0.8f;
        suite.check(game::evaluate_medals(r) & game::MedalSharpshooter, "accuracy -> sharpshooter");
    }
    {
        game::RunResult r;
        r.took_no_damage = true;
        suite.check(game::evaluate_medals(r) & game::MedalUntouchable, "no damage -> untouchable");
    }
    {
        game::RunResult r;
        r.max_multiplier = 5;
        suite.check(game::evaluate_medals(r) & game::MedalComboMaster, "big combo -> master");
    }
    {
        game::RunResult r;
        r.bosses_killed = 2;
        suite.check(game::evaluate_medals(r) & game::MedalBossSlayer, "boss kill -> slayer");
    }
    {
        game::RunResult r;
        r.kills = 100;
        suite.check(game::evaluate_medals(r) & game::MedalCenturion, "100 kills -> centurion");
    }
    {
        game::RunResult r;
        r.victory = true;
        suite.check(game::evaluate_medals(r) & game::MedalVictor, "victory -> victor");
    }

    // A flawless victory earns several at once.
    {
        game::RunResult r;
        r.accuracy = 0.9f;
        r.took_no_damage = true;
        r.victory = true;
        r.bosses_killed = 1;
        const u32 m = game::evaluate_medals(r);
        suite.check((m & game::MedalSharpshooter) && (m & game::MedalUntouchable)
                        && (m & game::MedalVictor) && (m & game::MedalBossSlayer),
                    "flawless victory earns multiple medals");
    }

    return suite.summary();
}
