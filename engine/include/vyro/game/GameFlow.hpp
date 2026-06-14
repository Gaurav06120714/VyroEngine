// VyroEngine — Game flow / objectives (V7.5)
// Turns "survive forever" into a structured run: each wave has a kill target,
// clearing it triggers a short intermission before the next wave, clearing the
// final wave is Victory, and losing all health is Defeat. A small explicit
// state machine — headless and tested; the game drives it and renders the HUD.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/game/WavePlan.hpp"

#include <vector>

namespace vyro::game {

enum class Phase : u8 {
    Fighting,     // killing the current wave
    Intermission, // brief pause before the next wave
    Victory,      // final wave cleared
    Defeat,       // player died
};

class GameFlow
{
public:
    GameFlow(int total_waves, int base_kills, int kills_per_wave, f32 intermission_time)
        : m_total(total_waves), m_base(base_kills), m_per_wave(kills_per_wave),
          m_intermission(intermission_time)
    {
        reset();
    }

    // Data-driven: an explicit per-wave plan (V8.5). Empty falls back to a
    // single default wave.
    explicit GameFlow(std::vector<WavePlan> plans) : m_plans(std::move(plans))
    {
        if (m_plans.empty()) {
            m_plans.push_back(WavePlan{});
        }
        m_total = static_cast<int>(m_plans.size());
        reset();
    }

    void reset()
    {
        m_phase = Phase::Fighting;
        m_wave = 1;
        m_kills = 0;
        m_timer = 0.0f;
    }

    // Kill target to clear the current wave.
    [[nodiscard]] int required() const
    {
        if (!m_plans.empty()) {
            const usize i = static_cast<usize>(m_wave - 1);
            return i < m_plans.size() ? m_plans[i].kills : m_plans.back().kills;
        }
        return m_base + (m_wave - 1) * m_per_wave;
    }

    void register_kill()
    {
        if (m_phase != Phase::Fighting) {
            return;
        }
        ++m_kills;
        if (m_kills >= required()) {
            if (m_wave >= m_total) {
                m_phase = Phase::Victory;
            } else {
                m_phase = Phase::Intermission;
                const usize i = static_cast<usize>(m_wave - 1);
                m_timer = !m_plans.empty() && i < m_plans.size() ? m_plans[i].intermission
                                                                 : m_intermission;
            }
        }
    }

    void player_died()
    {
        if (m_phase == Phase::Fighting || m_phase == Phase::Intermission) {
            m_phase = Phase::Defeat;
        }
    }

    void update(f32 dt)
    {
        if (m_phase != Phase::Intermission) {
            return;
        }
        m_timer -= dt;
        if (m_timer <= 0.0f) {
            ++m_wave;
            m_kills = 0;
            m_phase = Phase::Fighting;
        }
    }

    [[nodiscard]] Phase phase() const { return m_phase; }
    [[nodiscard]] int wave() const { return m_wave; }
    [[nodiscard]] int total_waves() const { return m_total; }
    [[nodiscard]] int kills() const { return m_kills; }
    [[nodiscard]] f32 intermission_left() const { return m_timer; }
    [[nodiscard]] bool spawning() const { return m_phase == Phase::Fighting; }
    [[nodiscard]] bool over() const { return m_phase == Phase::Victory || m_phase == Phase::Defeat; }

private:
    std::vector<WavePlan> m_plans; // data-driven plan (V8.5); empty = use formula
    int m_total = 1;
    int m_base = 5;
    int m_per_wave = 3;
    f32 m_intermission = 3.0f;
    Phase m_phase = Phase::Fighting;
    int m_wave = 1;
    int m_kills = 0;
    f32 m_timer = 0.0f;
};

} // namespace vyro::game
