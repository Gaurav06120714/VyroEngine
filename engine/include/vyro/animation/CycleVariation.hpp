// VyroEngine — Animation cycle variation (V7.1)
// GPU instancing draws the whole horde from one shared pose, so every zombie
// animates in lockstep. These helpers spread the horde across `buckets` phase
// offsets of a looping clip: each agent gets a stable bucket from its id, and
// each bucket plays the cycle shifted by a fraction of its duration. The game
// skins one pose per bucket and instances each bucket separately. Headless and
// tested.
#pragma once

#include "vyro/core/Types.hpp"

#include <cmath>

namespace vyro::anim {

// Stable bucket in [0, buckets) for an entity id.
[[nodiscard]] constexpr u32 phase_bucket(u32 id, u32 buckets)
{
    return buckets == 0 ? 0 : id % buckets;
}

// Playback time for `bucket`: the base time plus an even fraction of the clip
// duration, wrapped into [0, duration). Adjacent buckets are one slice apart.
[[nodiscard]] inline f32 bucket_time(u32 bucket, u32 buckets, f32 duration, f32 base_time = 0.0f)
{
    if (buckets == 0 || duration <= 0.0f) {
        return base_time;
    }
    const f32 offset = (static_cast<f32>(bucket) / static_cast<f32>(buckets)) * duration;
    f32 t = std::fmod(base_time + offset, duration);
    if (t < 0.0f) {
        t += duration;
    }
    return t;
}

} // namespace vyro::anim
