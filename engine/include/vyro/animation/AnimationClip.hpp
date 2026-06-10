// VyroEngine — Skeletal animation clip
// Phase 6.1: keyframed bone tracks sampled at a time to build a pose. Each
// track is a sequence of (time, value) keys interpolated linearly. Real
// skinning uploads the resulting transforms as a matrix palette.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

#include <string>
#include <vector>

namespace vyro {

struct Keyframe {
    f32 time = 0.0f;
    Vec3 value{};
};

class AnimationTrack
{
public:
    void add_key(f32 time, Vec3 value) { m_keys.push_back(Keyframe{time, value}); }

    // Sample the track, clamping before the first / after the last key.
    [[nodiscard]] Vec3 sample(f32 time) const;

    [[nodiscard]] usize key_count() const { return m_keys.size(); }

private:
    std::vector<Keyframe> m_keys;
};

class AnimationClip
{
public:
    AnimationClip() = default;
    AnimationClip(std::string name, f32 duration, bool looping)
        : m_name(std::move(name)), m_duration(duration), m_looping(looping)
    {
    }

    AnimationTrack& add_track() { return m_tracks.emplace_back(); }

    // Sample a track at `time`, wrapping the time if the clip loops.
    [[nodiscard]] Vec3 sample(usize track, f32 time) const;

    [[nodiscard]] const std::string& name() const { return m_name; }
    [[nodiscard]] f32 duration() const { return m_duration; }
    [[nodiscard]] bool looping() const { return m_looping; }
    [[nodiscard]] usize track_count() const { return m_tracks.size(); }

private:
    [[nodiscard]] f32 normalize_time(f32 time) const;

    std::string m_name;
    f32 m_duration = 0.0f;
    bool m_looping = false;
    std::vector<AnimationTrack> m_tracks;
};

} // namespace vyro
