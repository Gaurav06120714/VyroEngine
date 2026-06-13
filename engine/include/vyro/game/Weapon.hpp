// VyroEngine — Weapon model (V7.3)
// A data-driven firearm: fire rate, pellet count + spread, damage, magazine and
// reload. The Weapon runtime tracks ammo and cooldown/reload timers; firing is
// gated by them. `spread_angles` lays pellets evenly across the spread cone (for
// shotguns). Headless and tested — no rendering or input here.
#pragma once

#include "vyro/core/Types.hpp"

#include <vector>

namespace vyro::game {

struct WeaponStats {
    const char* name = "pistol";
    f32 fire_interval = 0.25f; // seconds between shots
    int pellets = 1;           // projectiles per shot
    f32 spread = 0.0f;         // total cone angle (radians) across pellets
    int damage = 1;            // damage per pellet
    int mag = 12;              // magazine size
    f32 reload_time = 1.0f;    // seconds to reload
};

// Evenly spaced pellet offset angles within [-spread/2, +spread/2]. One pellet
// fires straight ahead (0); the set is symmetric about 0.
[[nodiscard]] inline std::vector<f32> spread_angles(int pellets, f32 spread)
{
    std::vector<f32> out;
    if (pellets <= 1) {
        out.push_back(0.0f);
        return out;
    }
    out.reserve(static_cast<usize>(pellets));
    const f32 step = spread / static_cast<f32>(pellets - 1);
    for (int i = 0; i < pellets; ++i) {
        out.push_back(-spread * 0.5f + step * static_cast<f32>(i));
    }
    return out;
}

class Weapon
{
public:
    Weapon() = default;
    explicit Weapon(WeaponStats stats) : m_stats(stats), m_ammo(stats.mag) {}

    void update(f32 dt)
    {
        if (m_cooldown > 0.0f) {
            m_cooldown -= dt;
        }
        if (m_reloading) {
            m_reload_left -= dt;
            if (m_reload_left <= 0.0f) {
                m_ammo = m_stats.mag;
                m_reloading = false;
            }
        }
    }

    [[nodiscard]] bool can_fire() const
    {
        return !m_reloading && m_cooldown <= 0.0f && m_ammo > 0;
    }

    // Consume a round if able; returns true if a shot was fired.
    bool fire()
    {
        if (!can_fire()) {
            return false;
        }
        --m_ammo;
        m_cooldown = m_stats.fire_interval;
        return true;
    }

    void begin_reload()
    {
        if (!m_reloading && m_ammo < m_stats.mag) {
            m_reloading = true;
            m_reload_left = m_stats.reload_time;
        }
    }

    [[nodiscard]] int ammo() const { return m_ammo; }
    [[nodiscard]] bool reloading() const { return m_reloading; }
    [[nodiscard]] const WeaponStats& stats() const { return m_stats; }

private:
    WeaponStats m_stats{};
    int m_ammo = 0;
    f32 m_cooldown = 0.0f;
    bool m_reloading = false;
    f32 m_reload_left = 0.0f;
};

} // namespace vyro::game
