// VyroEngine — Pickups & power-ups (V8.2)
// Kills may drop a pickup; the kind is a weighted choice. Pure decision helpers
// (drop gate + weighted kind) plus the effect amounts, kept headless and tested.
#pragma once

#include "vyro/core/Random.hpp"
#include "vyro/core/Types.hpp"

#include <array>

namespace vyro::game {

enum class PickupKind : u8 {
    Health,     // restores a heart
    Ammo,       // refills the current weapon
    ScoreBoost, // bonus points
};

struct PickupTable {
    f32 drop_chance = 0.25f; // probability a kill drops anything
    f32 w_health = 30.0f;
    f32 w_ammo = 50.0f;
    f32 w_score = 20.0f;
};

// Effect magnitudes.
inline constexpr int kHealthRestore = 1;
inline constexpr int kScoreBoost = 25;

// Does this kill drop a pickup? `roll` in [0,1).
[[nodiscard]] inline bool should_drop(f32 roll, f32 chance)
{
    return roll < chance;
}

// Which kind drops, by weighted choice. `roll` in [0,1).
[[nodiscard]] inline PickupKind pick_kind(const PickupTable& t, f32 roll)
{
    const std::array<f32, 3> w{t.w_health, t.w_ammo, t.w_score};
    return static_cast<PickupKind>(weighted_index(w, roll));
}

} // namespace vyro::game
