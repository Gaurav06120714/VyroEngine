// VyroEngine — Boss schedule tests (V9.3)
#include "vyro/game/BossSchedule.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("boss");

    // Every 3rd wave is a boss wave.
    suite.check(!game::is_boss_wave(0, 3), "wave 0 is never a boss wave");
    suite.check(!game::is_boss_wave(1, 3) && !game::is_boss_wave(2, 3), "early waves are normal");
    suite.check(game::is_boss_wave(3, 3) && game::is_boss_wave(6, 3), "every 3rd is a boss wave");
    suite.check(!game::is_boss_wave(4, 3), "between boss waves is normal");
    suite.check(!game::is_boss_wave(3, 0), "zero interval never triggers");

    // Boss stats scale with the wave and are tougher/bigger than a grunt.
    {
        const auto b3 = game::boss_for_wave(3);
        const auto b6 = game::boss_for_wave(6);
        suite.check(b3.health > 1 && b3.scale > 1.5f, "boss is tough and large");
        suite.check(b6.health > b3.health, "later bosses are tougher");
        suite.check(b6.score > b3.score && b3.credits == b3.score, "rewards scale, credits=score");
        suite.check(b3.speed_mult < 1.0f, "bosses are slower than grunts");
    }

    return suite.summary();
}
