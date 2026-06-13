// VyroEngine — Procedural sound synthesis (V3.3)
// Generates game sound effects as mono float PCM at 44.1 kHz — no audio files
// required. The AudioDevice plays these buffers; tests verify them headlessly.
#pragma once

#include "vyro/core/Types.hpp"

#include <vector>

namespace vyro::synth {

inline constexpr u32 kSampleRate = 44100;

// A short noise burst with a sharp exponential decay (rifle shot).
[[nodiscard]] std::vector<f32> gunshot(f32 seconds = 0.18f);

// A low wobbling tone with slow decay (zombie groan).
[[nodiscard]] std::vector<f32> groan(f32 seconds = 0.7f);

// A quick descending blip (player hit / bite).
[[nodiscard]] std::vector<f32> hit(f32 seconds = 0.25f);

// A seamless, loopable dark synth bass/arpeggio bed for background music.
// The returned buffer is an exact number of beats so it loops cleanly.
[[nodiscard]] std::vector<f32> music_loop(f32 bpm = 96.0f, u32 bars = 4);

// Peak absolute amplitude of a buffer (test/inspection helper).
[[nodiscard]] f32 peak(const std::vector<f32>& samples);

} // namespace vyro::synth
