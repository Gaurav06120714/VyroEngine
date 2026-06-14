// VyroEngine — Run statistics (V8.3)
// Accumulates a run's performance — shots, hits, kills (by archetype) and time
// survived — and derives accuracy for the end-of-run summary. Headless and
// tested; the game feeds it events and reads it on the results screen.
#pragma once

#include "vyro/core/Types.hpp"

#include <array>

namespace vyro::game {

inline constexpr usize kMaxEnemyTypes = 8;

struct RunStats {
    int shots = 0; // projectiles fired
    int hits = 0;  // projectiles that connected
    int kills = 0;
    std::array<int, kMaxEnemyTypes> kills_by_type{};
    f32 time = 0.0f; // seconds survived

    void on_shots(int n) { shots += n; }
    void on_hit() { ++hits; }
    void on_kill(int type)
    {
        ++kills;
        if (type >= 0 && static_cast<usize>(type) < kMaxEnemyTypes) {
            ++kills_by_type[static_cast<usize>(type)];
        }
    }
    void tick(f32 dt) { time += dt; }

    // Fraction of fired projectiles that hit, in [0,1] (0 when nothing fired).
    [[nodiscard]] f32 accuracy() const
    {
        return shots > 0 ? static_cast<f32>(hits) / static_cast<f32>(shots) : 0.0f;
    }

    void reset() { *this = RunStats{}; }
};

} // namespace vyro::game
