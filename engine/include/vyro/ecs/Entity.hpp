// VyroEngine — Entity identity
// Phase 2.1: a lightweight handle of (index, generation). The generation
// invalidates stale handles when an index is recycled, preventing use of a
// destroyed entity at the gameplay level.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro {

inline constexpr u32 kInvalidEntityIndex = 0xFFFFFFFFu;

struct Entity {
    u32 index = kInvalidEntityIndex;
    u32 generation = 0;

    [[nodiscard]] constexpr bool is_null() const { return index == kInvalidEntityIndex; }

    friend constexpr bool operator==(const Entity& a, const Entity& b)
    {
        return a.index == b.index && a.generation == b.generation;
    }
    friend constexpr bool operator!=(const Entity& a, const Entity& b) { return !(a == b); }
};

inline constexpr Entity kNullEntity{};

} // namespace vyro
