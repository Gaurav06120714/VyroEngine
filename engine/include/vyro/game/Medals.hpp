// VyroEngine — Medals & milestones (V9.5)
// Evaluates which medals a finished run earns from its result, as a bitmask so
// the profile can accumulate them across runs. Pure logic — headless and tested.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro::game {

enum MedalBit : u32 {
    MedalSharpshooter = 1u << 0, // high accuracy
    MedalUntouchable = 1u << 1,  // finished without taking damage
    MedalComboMaster = 1u << 2,  // reached a big combo
    MedalBossSlayer = 1u << 3,   // killed at least one boss
    MedalCenturion = 1u << 4,    // 100+ kills
    MedalVictor = 1u << 5,       // cleared every wave
};

struct RunResult {
    f32 accuracy = 0.0f;
    int max_multiplier = 1;
    bool took_no_damage = false;
    int bosses_killed = 0;
    int kills = 0;
    bool victory = false;
};

// The medals earned by a run, as an OR of MedalBit flags.
[[nodiscard]] inline u32 evaluate_medals(const RunResult& r)
{
    u32 m = 0;
    if (r.accuracy >= 0.75f) {
        m |= MedalSharpshooter;
    }
    if (r.took_no_damage) {
        m |= MedalUntouchable;
    }
    if (r.max_multiplier >= 5) {
        m |= MedalComboMaster;
    }
    if (r.bosses_killed >= 1) {
        m |= MedalBossSlayer;
    }
    if (r.kills >= 100) {
        m |= MedalCenturion;
    }
    if (r.victory) {
        m |= MedalVictor;
    }
    return m;
}

[[nodiscard]] inline const char* medal_name(MedalBit bit)
{
    switch (bit) {
    case MedalSharpshooter:
        return "SHARPSHOOTER";
    case MedalUntouchable:
        return "UNTOUCHABLE";
    case MedalComboMaster:
        return "COMBO MASTER";
    case MedalBossSlayer:
        return "BOSS SLAYER";
    case MedalCenturion:
        return "CENTURION";
    case MedalVictor:
        return "VICTOR";
    }
    return "?";
}

} // namespace vyro::game
