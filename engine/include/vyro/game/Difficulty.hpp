// VyroEngine — Difficulty modes (V8.4)
// Easy / Normal / Hard scaling applied to enemy speed, enemy health, spawn rate
// and starting player health. Selected at start, saved in the profile. A pure
// lookup table — headless and tested.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro::game {

enum class Difficulty : u8 {
    Easy = 0,
    Normal = 1,
    Hard = 2,
};

struct DifficultyMods {
    const char* name = "NORMAL";
    f32 enemy_speed = 1.0f;  // multiplier
    f32 enemy_health = 1.0f; // multiplier (rounded, min 1)
    f32 spawn_rate = 1.0f;   // >1 spawns faster
    int player_hp = 3;
};

// Mods for a difficulty index; out-of-range falls back to Normal.
[[nodiscard]] inline DifficultyMods difficulty_mods(int level)
{
    switch (level) {
    case 0:
        return DifficultyMods{"EASY", 0.8f, 0.7f, 0.8f, 5};
    case 2:
        return DifficultyMods{"HARD", 1.3f, 1.6f, 1.3f, 2};
    case 1:
    default:
        return DifficultyMods{"NORMAL", 1.0f, 1.0f, 1.0f, 3};
    }
}

// Cycle Easy -> Normal -> Hard -> Easy.
[[nodiscard]] inline int next_difficulty(int level)
{
    return (level + 1) % 3;
}

} // namespace vyro::game
