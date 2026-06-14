// VyroEngine — Combo multiplier (V9.4)
// Rapid kills build a streak that grants a score/credit multiplier; letting the
// window lapse without a kill resets the streak. Headless and tested.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro::game {

class Combo
{
public:
    explicit Combo(f32 window_seconds = 2.5f, int kills_per_tier = 3, int max_multiplier = 5)
        : m_window(window_seconds), m_per_tier(kills_per_tier < 1 ? 1 : kills_per_tier),
          m_max(max_multiplier < 1 ? 1 : max_multiplier)
    {
    }

    // Register a kill: extend the streak and refresh the window.
    void on_kill()
    {
        ++m_count;
        m_timer = m_window;
    }

    // Decay: when the window lapses without a kill, the streak breaks.
    void update(f32 dt)
    {
        if (m_count == 0) {
            return;
        }
        m_timer -= dt;
        if (m_timer <= 0.0f) {
            m_count = 0;
        }
    }

    [[nodiscard]] int count() const { return m_count; }

    // 1x baseline, +1 every `kills_per_tier` kills, capped at `max_multiplier`.
    [[nodiscard]] int multiplier() const
    {
        const int m = 1 + m_count / m_per_tier;
        return m > m_max ? m_max : m;
    }

    void reset()
    {
        m_count = 0;
        m_timer = 0.0f;
    }

private:
    f32 m_window = 2.5f;
    int m_per_tier = 3;
    int m_max = 5;
    int m_count = 0;
    f32 m_timer = 0.0f;
};

} // namespace vyro::game
