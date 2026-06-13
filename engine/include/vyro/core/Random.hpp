// VyroEngine — Weighted selection (V7.2)
// A tiny, deterministic helper for spawn tables and loot: given per-entry
// weights and a roll in [0,1), return the chosen index. Used by the game to
// pick enemy archetypes (and reusable for any weighted choice). Headless and
// tested.
#pragma once

#include "vyro/core/Types.hpp"

#include <span>

namespace vyro {

// Index whose cumulative weight bracket contains `roll01` * total_weight.
// Negative weights are treated as 0; an all-zero/empty table returns 0.
[[nodiscard]] inline usize weighted_index(std::span<const f32> weights, f32 roll01)
{
    f32 total = 0.0f;
    for (const f32 w : weights) {
        total += w > 0.0f ? w : 0.0f;
    }
    if (total <= 0.0f || weights.empty()) {
        return 0;
    }
    const f32 r = (roll01 < 0.0f ? 0.0f : (roll01 > 1.0f ? 1.0f : roll01)) * total;
    f32 acc = 0.0f;
    for (usize i = 0; i < weights.size(); ++i) {
        acc += weights[i] > 0.0f ? weights[i] : 0.0f;
        if (r < acc) {
            return i;
        }
    }
    return weights.size() - 1;
}

} // namespace vyro
