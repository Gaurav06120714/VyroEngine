// VyroEngine — Boss schedule (V9.3)
// Decides which waves are boss waves and how tough the boss is. A boss is a
// single large, high-health enemy worth big score + credits. Pure functions —
// headless and tested.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro::game {

struct BossStats {
    int health = 20;
    f32 scale = 2.5f;
    int score = 50;
    int credits = 50;
    f32 speed_mult = 0.5f; // bosses are slow but relentless
};

// Every `every` waves is a boss wave (wave 0 is never one).
[[nodiscard]] inline bool is_boss_wave(int wave, int every)
{
    return wave > 0 && every > 0 && (wave % every) == 0;
}

// Boss stats scale with the wave number.
[[nodiscard]] inline BossStats boss_for_wave(int wave)
{
    BossStats b;
    b.health = 20 + wave * 12;
    b.score = 50 + wave * 15;
    b.credits = b.score;
    b.scale = 2.5f;
    b.speed_mult = 0.5f;
    return b;
}

} // namespace vyro::game
