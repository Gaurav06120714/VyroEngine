// VyroEngine — Procedural sound synthesis implementation
#include "vyro/audio/SoundSynth.hpp"

#include <cmath>

namespace vyro::synth {

namespace {

// Deterministic xorshift noise so generated sounds are reproducible.
f32 noise(u32& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (static_cast<f32>(state & 0xFFFFFF) / static_cast<f32>(0xFFFFFF)) * 2.0f - 1.0f;
}

constexpr f32 kTau = 6.28318530718f;

} // namespace

std::vector<f32> gunshot(f32 seconds)
{
    const usize n = static_cast<usize>(seconds * kSampleRate);
    std::vector<f32> out(n);
    u32 rng = 0xC0FFEE;
    for (usize i = 0; i < n; ++i) {
        const f32 t = static_cast<f32>(i) / kSampleRate;
        const f32 envelope = std::exp(-t * 38.0f);
        // Noise body plus a low "thump" that decays faster.
        const f32 body = noise(rng) * envelope;
        const f32 thump = std::sin(kTau * 70.0f * t) * std::exp(-t * 60.0f);
        out[i] = 0.8f * body + 0.6f * thump;
    }
    return out;
}

std::vector<f32> groan(f32 seconds)
{
    const usize n = static_cast<usize>(seconds * kSampleRate);
    std::vector<f32> out(n);
    u32 rng = 0xDEAD;
    for (usize i = 0; i < n; ++i) {
        const f32 t = static_cast<f32>(i) / kSampleRate;
        const f32 envelope = std::exp(-t * 3.0f) * (1.0f - std::exp(-t * 40.0f));
        // Pitch wobbles slowly downward; a little breath noise on top.
        const f32 freq = 95.0f - 25.0f * t + 14.0f * std::sin(kTau * 4.5f * t);
        const f32 voice = std::sin(kTau * freq * t) + 0.45f * std::sin(kTau * freq * 2.02f * t);
        out[i] = (0.55f * voice + 0.1f * noise(rng)) * envelope;
    }
    return out;
}

std::vector<f32> hit(f32 seconds)
{
    const usize n = static_cast<usize>(seconds * kSampleRate);
    std::vector<f32> out(n);
    for (usize i = 0; i < n; ++i) {
        const f32 t = static_cast<f32>(i) / kSampleRate;
        const f32 envelope = std::exp(-t * 16.0f);
        const f32 freq = 480.0f - 320.0f * (t / seconds); // falling pitch
        out[i] = 0.7f * std::sin(kTau * freq * t) * envelope;
    }
    return out;
}

std::vector<f32> music_loop(f32 bpm, u32 bars)
{
    const f32 beat = 60.0f / bpm;                  // seconds per beat
    const u32 beats = bars * 4;                    // 4/4 time
    const usize n = static_cast<usize>(static_cast<f32>(beats) * beat * kSampleRate);
    std::vector<f32> out(n);

    // A minor-key bass riff (one note per beat) with a soft arpeggio on top.
    const f32 bass[8] = {55.0f, 55.0f, 65.41f, 49.0f, 55.0f, 73.42f, 65.41f, 49.0f}; // A1..
    const f32 arp[4] = {220.0f, 261.63f, 329.63f, 261.63f}; // A3 C4 E4 C4

    for (usize i = 0; i < n; ++i) {
        const f32 t = static_cast<f32>(i) / kSampleRate;
        const f32 beat_pos = t / beat;
        const u32 beat_idx = static_cast<u32>(beat_pos) % 8;
        const f32 beat_frac = beat_pos - std::floor(beat_pos);

        // Plucky envelope per beat.
        const f32 env = std::exp(-beat_frac * 3.0f);
        const f32 b = std::sin(kTau * bass[beat_idx] * t)
                      + 0.4f * std::sin(kTau * bass[beat_idx] * 2.0f * t);
        const f32 a = std::sin(kTau * arp[static_cast<u32>(beat_pos * 2.0f) % 4] * t);

        out[i] = 0.32f * b * env + 0.12f * a * std::exp(-beat_frac * 6.0f);
    }
    return out;
}

f32 peak(const std::vector<f32>& samples)
{
    f32 best = 0.0f;
    for (const f32 s : samples) {
        best = std::max(best, std::fabs(s));
    }
    return best;
}

} // namespace vyro::synth
