// VyroEngine — Audio clip and bus types
// Phase 5.4 / 5.2: a decoded clip's metadata and the mixer bus hierarchy.
#pragma once

#include "vyro/core/Types.hpp"

#include <string>

namespace vyro {

// Mixer routing buses. Master scales everything; sub-buses group categories.
enum class AudioBus : u8 {
    Master,
    Music,
    Sfx,

    Count,
};

inline constexpr usize kAudioBusCount = static_cast<usize>(AudioBus::Count);

struct AudioClip {
    std::string name;
    u32 sample_rate = 44100;
    u16 channels = 2;
    u32 frame_count = 0; // samples per channel

    [[nodiscard]] f32 duration() const
    {
        return sample_rate > 0 ? static_cast<f32>(frame_count) / static_cast<f32>(sample_rate) : 0.0f;
    }
};

} // namespace vyro
