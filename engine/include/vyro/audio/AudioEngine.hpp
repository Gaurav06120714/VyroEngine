// VyroEngine — Audio engine
// Phase 5: manages clips (5.4 resource management), plays and advances sources
// (5.1 playback), combines master/bus/source gains (5.2 mixing), and applies
// distance attenuation and panning from a listener (5.3 spatial audio). The
// model is device-agnostic and deterministic; a real backend (OpenAL /
// miniaudio) consumes the computed gains to render samples.
#pragma once

#include "vyro/audio/AudioClip.hpp"
#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

#include <array>
#include <string_view>
#include <vector>

namespace vyro {

using ClipId = u32;
using SourceId = u32;

inline constexpr ClipId kInvalidClip = 0xFFFFFFFFu;
inline constexpr SourceId kInvalidSource = 0xFFFFFFFFu;

class AudioEngine : NonCopyable
{
public:
    AudioEngine();

    // ── Resource management (5.4) ────────────────────────────────────
    ClipId load_clip(std::string_view name, u32 sample_rate, u16 channels, u32 frame_count);
    [[nodiscard]] const AudioClip* clip(ClipId id) const;
    [[nodiscard]] usize clip_count() const { return m_clips.size(); }

    // ── Playback (5.1) ───────────────────────────────────────────────
    SourceId play(ClipId clip, f32 volume = 1.0f, AudioBus bus = AudioBus::Sfx, bool looping = false);
    SourceId play_spatial(ClipId clip, Vec3 position, f32 volume = 1.0f, bool looping = false);
    void stop(SourceId source);
    [[nodiscard]] bool is_playing(SourceId source) const;

    // Advance playheads; non-looping sources stop when they reach the end.
    void update(f32 dt);

    // ── Mixing (5.2) ─────────────────────────────────────────────────
    void set_master_volume(f32 volume) { m_master = clamp01(volume); }
    void set_bus_volume(AudioBus bus, f32 volume) { m_bus_volume[static_cast<usize>(bus)] = clamp01(volume); }
    [[nodiscard]] f32 master_volume() const { return m_master; }
    [[nodiscard]] f32 bus_volume(AudioBus bus) const { return m_bus_volume[static_cast<usize>(bus)]; }

    // ── Spatial (5.3) ────────────────────────────────────────────────
    void set_listener(Vec3 position) { m_listener = position; }
    void set_attenuation(f32 ref_distance, f32 max_distance);

    // Final per-source linear gain (volume * bus * master * attenuation).
    [[nodiscard]] f32 effective_gain(SourceId source) const;
    // Stereo pan in [-1, 1] for spatial sources (0 for non-spatial).
    [[nodiscard]] f32 pan(SourceId source) const;

    [[nodiscard]] u32 active_sources() const;

private:
    struct Source {
        ClipId clip = kInvalidClip;
        f32 volume = 1.0f;
        AudioBus bus = AudioBus::Sfx;
        bool looping = false;
        bool spatial = false;
        Vec3 position{};
        f32 playhead = 0.0f;
        bool active = false;
    };

    [[nodiscard]] static f32 clamp01(f32 v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
    [[nodiscard]] f32 attenuation_for(Vec3 position) const;

    std::vector<AudioClip> m_clips;
    std::vector<Source> m_sources;
    std::array<f32, kAudioBusCount> m_bus_volume{};
    f32 m_master = 1.0f;
    Vec3 m_listener{};
    f32 m_ref_distance = 1.0f;
    f32 m_max_distance = 100.0f;
};

} // namespace vyro
