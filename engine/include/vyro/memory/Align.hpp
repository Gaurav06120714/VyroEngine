// VyroEngine — Memory alignment helpers
// Phase 1.6: shared utilities for the allocator family.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro::memory {

// Round `value` up to the nearest multiple of `alignment` (a power of two).
[[nodiscard]] inline constexpr usize align_up(usize value, usize alignment)
{
    return (value + (alignment - 1)) & ~(alignment - 1);
}

// Bytes needed to advance `address` to the next `alignment` boundary.
[[nodiscard]] inline usize align_adjustment(const void* address, usize alignment)
{
    const auto raw = reinterpret_cast<usize>(address);
    const usize aligned = align_up(raw, alignment);
    return aligned - raw;
}

[[nodiscard]] inline constexpr bool is_power_of_two(usize value)
{
    return value != 0 && (value & (value - 1)) == 0;
}

} // namespace vyro::memory
