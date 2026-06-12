// VyroEngine — Real audio output device (V3.3)
// A miniaudio-backed playback device: fire-and-forget PCM voices mixed in the
// audio callback. The v1 AudioEngine model computes gains; this device makes
// them audible. Built as the optional vyro_audio target.
#pragma once

#include "vyro/core/Types.hpp"

#include <memory>
#include <vector>

namespace vyro {

class AudioDevice
{
public:
    AudioDevice();
    ~AudioDevice();

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    // Open the default output device (44.1 kHz mono mix, stereo out).
    bool init();
    void shutdown();
    [[nodiscard]] bool is_open() const;

    // Start a voice playing `samples` (mono 44.1 kHz floats) at `gain`.
    // The samples are copied; the voice frees itself when finished.
    void play(const std::vector<f32>& samples, f32 gain = 1.0f);

    // Voices still audible right now.
    [[nodiscard]] u32 active_voices() const;

    // Master output gain (clamped to [0, 1]).
    void set_master_gain(f32 gain);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vyro
