// VyroEngine — Boss phases (V10.2)
// A boss escalates as its health drops: calm -> enraged -> frenzied, each phase
// faster than the last. Derived purely from the health fraction — headless and
// tested.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro::game {

enum class BossPhase : u8 {
    Calm = 0,     // > 66% health
    Enraged = 1,  // 33%–66%
    Frenzied = 2, // < 33%
};

[[nodiscard]] inline BossPhase boss_phase(int health, int max_health)
{
    if (max_health <= 0) {
        return BossPhase::Calm;
    }
    const f32 frac = static_cast<f32>(health) / static_cast<f32>(max_health);
    if (frac > 0.66f) {
        return BossPhase::Calm;
    }
    if (frac > 0.33f) {
        return BossPhase::Enraged;
    }
    return BossPhase::Frenzied;
}

// Speed multiplier for a phase — bosses get faster as they enrage.
[[nodiscard]] inline f32 phase_speed_mult(BossPhase phase)
{
    switch (phase) {
    case BossPhase::Enraged:
        return 1.5f;
    case BossPhase::Frenzied:
        return 2.2f;
    case BossPhase::Calm:
    default:
        return 1.0f;
    }
}

[[nodiscard]] inline const char* phase_name(BossPhase phase)
{
    switch (phase) {
    case BossPhase::Enraged:
        return "ENRAGED";
    case BossPhase::Frenzied:
        return "FRENZIED";
    case BossPhase::Calm:
    default:
        return "BOSS";
    }
}

} // namespace vyro::game
