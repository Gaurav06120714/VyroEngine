// VyroEngine — Spatial audio gain (V7.4)
// Pure helpers to place a sound in the world relative to a listener: distance
// attenuation (full within `min`, fading to silence by `max`) and a stereo
// pan/gain from the listener's right axis. Headless and tested; the game uses
// the attenuation to drive its mono device today and the pan feeds a future
// stereo output.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

namespace vyro::audio {

struct StereoGain {
    f32 left = 1.0f;
    f32 right = 1.0f;
};

// 1 at distance <= min, linearly down to 0 at distance >= max.
[[nodiscard]] inline f32 attenuation(f32 distance, f32 min_dist, f32 max_dist)
{
    if (distance <= min_dist) {
        return 1.0f;
    }
    if (distance >= max_dist || max_dist <= min_dist) {
        return distance >= max_dist ? 0.0f : 1.0f;
    }
    return 1.0f - (distance - min_dist) / (max_dist - min_dist);
}

// Pan in [-1, 1]: -1 fully left, +1 fully right, from the source's projection
// onto the listener's right axis.
[[nodiscard]] inline f32 pan(Vec3 listener, Vec3 listener_right, Vec3 source)
{
    const Vec3 to = source - listener;
    const f32 len = length(to);
    if (len <= 1e-5f) {
        return 0.0f;
    }
    const f32 p = dot(to, normalize(listener_right)) / len; // cos angle to right
    return p < -1.0f ? -1.0f : (p > 1.0f ? 1.0f : p);
}

// Combined per-ear gain: distance attenuation split by an equal-power pan.
[[nodiscard]] inline StereoGain spatial_gain(Vec3 listener, Vec3 listener_right, Vec3 source,
                                             f32 min_dist, f32 max_dist)
{
    const f32 a = attenuation(length(source - listener), min_dist, max_dist);
    const f32 p = pan(listener, listener_right, source);   // -1..1
    const f32 t = (p + 1.0f) * 0.5f;                       // 0..1 (left..right)
    return StereoGain{a * (1.0f - t), a * t};
}

} // namespace vyro::audio
