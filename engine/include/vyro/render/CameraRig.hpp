// VyroEngine — Camera rig (V4.5)
// Gameplay-facing camera helpers, kept headless and deterministic so they can
// be unit-tested without a GPU:
//   * smooth_follow  — frame-rate-independent exponential smoothing toward a
//                      target, for a follow camera that eases instead of snaps.
//   * ScreenShake    — trauma-based shake (Game Feel / Squirrel Eiserloh style):
//                      callers add trauma on impacts, it decays over time, and
//                      the offset scales with trauma squared so small hits are
//                      gentle and big ones kick hard.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

#include <cmath>

namespace vyro {

// Fraction to lerp this frame for an exponential approach with the given
// stiffness (1/seconds). Frame-rate independent: 1 - e^(-k·dt), in [0, 1].
[[nodiscard]] inline f32 smooth_factor(f32 stiffness, f32 dt)
{
    if (stiffness <= 0.0f || dt <= 0.0f) {
        return 0.0f;
    }
    const f32 a = 1.0f - std::exp(-stiffness * dt);
    return a > 1.0f ? 1.0f : a;
}

// Ease `current` toward `target`; equal positions stay put.
[[nodiscard]] inline Vec3 smooth_follow(Vec3 current, Vec3 target, f32 stiffness, f32 dt)
{
    const f32 a = smooth_factor(stiffness, dt);
    return current + (target - current) * a;
}

class ScreenShake
{
public:
    explicit ScreenShake(f32 decay_per_second = 1.5f) : m_decay(decay_per_second) {}

    // Add trauma from an impact; total trauma saturates at 1.
    void add_trauma(f32 amount)
    {
        m_trauma += amount;
        if (m_trauma > 1.0f) {
            m_trauma = 1.0f;
        }
        if (m_trauma < 0.0f) {
            m_trauma = 0.0f;
        }
    }

    // Decay trauma and advance the deterministic noise sequence.
    void update(f32 dt)
    {
        m_trauma -= m_decay * dt;
        if (m_trauma < 0.0f) {
            m_trauma = 0.0f;
        }
        ++m_tick;
    }

    [[nodiscard]] f32 trauma() const { return m_trauma; }

    // Positional shake offset, magnitude up to `max_translation` on x/y. Shake
    // = trauma², so it ramps non-linearly. Deterministic for a given tick.
    [[nodiscard]] Vec3 offset(f32 max_translation) const
    {
        const f32 shake = m_trauma * m_trauma;
        const f32 mag = max_translation * shake;
        return Vec3{mag * noise(0), mag * noise(1), 0.0f};
    }

private:
    // Cheap deterministic noise in [-1, 1] from the tick and an axis seed.
    [[nodiscard]] f32 noise(u32 axis) const
    {
        const f32 s = std::sin(static_cast<f32>(m_tick) * 12.9898f
                               + static_cast<f32>(axis) * 78.233f)
                      * 43758.5453f;
        return 2.0f * (s - std::floor(s)) - 1.0f;
    }

    f32 m_trauma = 0.0f;
    f32 m_decay = 1.5f;
    u32 m_tick = 0;
};

} // namespace vyro
