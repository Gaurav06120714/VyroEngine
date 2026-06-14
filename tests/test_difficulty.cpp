// VyroEngine — Difficulty mode tests (V8.4)
#include "vyro/game/Difficulty.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("difficulty");

    const auto easy = game::difficulty_mods(0);
    const auto normal = game::difficulty_mods(1);
    const auto hard = game::difficulty_mods(2);

    suite.check(normal.enemy_speed == 1.0f && normal.player_hp == 3, "normal is the baseline");
    suite.check(easy.enemy_speed < normal.enemy_speed, "easy enemies are slower");
    suite.check(hard.enemy_speed > normal.enemy_speed, "hard enemies are faster");
    suite.check(easy.player_hp > hard.player_hp, "easy gives more health than hard");
    suite.check(hard.enemy_health > normal.enemy_health, "hard enemies are tougher");

    // Out-of-range falls back to Normal.
    suite.check(game::difficulty_mods(99).player_hp == 3, "unknown level falls back to normal");

    // Cycling wraps Easy -> Normal -> Hard -> Easy.
    suite.check(game::next_difficulty(0) == 1 && game::next_difficulty(1) == 2
                    && game::next_difficulty(2) == 0,
                "difficulty cycles and wraps");

    return suite.summary();
}
