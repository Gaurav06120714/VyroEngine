// VyroEngine — Skeletal animation clip implementation
#include "vyro/animation/AnimationClip.hpp"

#include <cmath>

namespace vyro {

namespace {
Vec3 lerp(Vec3 a, Vec3 b, f32 t) { return a + (b - a) * t; }
} // namespace

Vec3 AnimationTrack::sample(f32 time) const
{
    if (m_keys.empty()) {
        return Vec3{};
    }
    if (time <= m_keys.front().time) {
        return m_keys.front().value;
    }
    if (time >= m_keys.back().time) {
        return m_keys.back().value;
    }

    // Find the bracketing pair and interpolate.
    for (usize i = 1; i < m_keys.size(); ++i) {
        if (time < m_keys[i].time) {
            const Keyframe& k0 = m_keys[i - 1];
            const Keyframe& k1 = m_keys[i];
            const f32 span = k1.time - k0.time;
            const f32 t = span > 0.0f ? (time - k0.time) / span : 0.0f;
            return lerp(k0.value, k1.value, t);
        }
    }
    return m_keys.back().value;
}

f32 AnimationClip::normalize_time(f32 time) const
{
    if (m_looping && m_duration > 0.0f) {
        time = std::fmod(time, m_duration);
        if (time < 0.0f) {
            time += m_duration;
        }
    }
    return time;
}

Vec3 AnimationClip::sample(usize track, f32 time) const
{
    if (track >= m_tracks.size()) {
        return Vec3{};
    }
    return m_tracks[track].sample(normalize_time(time));
}

} // namespace vyro
