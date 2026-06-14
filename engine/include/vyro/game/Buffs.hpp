// VyroEngine — Timed power-up buffs (V10.4)
// Temporary effects granted by special pickups: rapid fire and double damage,
// each with its own countdown. Exposes the multipliers the game applies.
// Headless and tested.
#pragma once

#include "vyro/core/Types.hpp"

#include <array>

namespace vyro::game {

enum class BuffKind : u8 {
    RapidFire = 0,
    DoubleDamage = 1,
};
inline constexpr int kBuffKinds = 2;

class Buffs
{
public:
    // Grant (or refresh) a buff for `duration` seconds.
    void grant(BuffKind k, f32 duration)
    {
        f32& t = m_time[idx(k)];
        if (duration > t) {
            t = duration;
        }
    }

    void update(f32 dt)
    {
        for (f32& t : m_time) {
            if (t > 0.0f) {
                t -= dt;
                if (t < 0.0f) {
                    t = 0.0f;
                }
            }
        }
    }

    [[nodiscard]] bool active(BuffKind k) const { return m_time[idx(k)] > 0.0f; }
    [[nodiscard]] f32 remaining(BuffKind k) const { return m_time[idx(k)]; }

    [[nodiscard]] f32 fire_rate_mult() const { return active(BuffKind::RapidFire) ? 2.0f : 1.0f; }
    [[nodiscard]] int damage_mult() const { return active(BuffKind::DoubleDamage) ? 2 : 1; }

    void reset() { m_time = {}; }

private:
    [[nodiscard]] static usize idx(BuffKind k) { return static_cast<usize>(k); }
    std::array<f32, kBuffKinds> m_time{};
};

} // namespace vyro::game
