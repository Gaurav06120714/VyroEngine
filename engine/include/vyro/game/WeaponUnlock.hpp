// VyroEngine — Weapon unlocks (V10.3)
// Loadouts unlock as the player reaches later waves. Pure gating logic — which
// weapon indices are available at a given wave. Headless and tested.
#pragma once

#include "vyro/core/Types.hpp"

#include <array>

namespace vyro::game {

// Wave at which each weapon (by index) becomes available. Pistol from the
// start; rifle at wave 2; shotgun at wave 3.
inline constexpr std::array<int, 3> kWeaponUnlockWave{1, 2, 3};

[[nodiscard]] inline bool weapon_unlocked(int weapon_index, int wave_reached)
{
    if (weapon_index < 0 || static_cast<usize>(weapon_index) >= kWeaponUnlockWave.size()) {
        return false;
    }
    return wave_reached >= kWeaponUnlockWave[static_cast<usize>(weapon_index)];
}

// How many weapons are unlocked at `wave_reached`.
[[nodiscard]] inline int unlocked_count(int wave_reached)
{
    int n = 0;
    for (usize i = 0; i < kWeaponUnlockWave.size(); ++i) {
        if (wave_reached >= kWeaponUnlockWave[i]) {
            ++n;
        }
    }
    return n;
}

} // namespace vyro::game
