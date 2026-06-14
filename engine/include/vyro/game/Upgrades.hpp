// VyroEngine — Player upgrades (V9.2)
// Tiered, capped upgrades bought with credits between waves: damage, fire rate,
// max health and move speed. Costs rise per tier; getters expose the modifiers
// the game applies. Headless and tested.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/game/Economy.hpp"

#include <array>

namespace vyro::game {

enum class UpgradeKind : u8 {
    Damage = 0,
    FireRate = 1,
    MaxHealth = 2,
    MoveSpeed = 3,
};
inline constexpr int kUpgradeKinds = 4;
inline constexpr int kMaxTier = 5;

class Upgrades
{
public:
    [[nodiscard]] int tier(UpgradeKind k) const { return m_tier[idx(k)]; }
    [[nodiscard]] bool maxed(UpgradeKind k) const { return m_tier[idx(k)] >= kMaxTier; }

    // Cost of the NEXT tier (rises with the current tier).
    [[nodiscard]] int cost(UpgradeKind k) const { return 50 * (m_tier[idx(k)] + 1); }

    // Buy the next tier if not maxed and affordable; returns true on success.
    bool buy(Economy& economy, UpgradeKind k)
    {
        if (maxed(k) || !economy.spend(cost(k))) {
            return false;
        }
        ++m_tier[idx(k)];
        return true;
    }

    // Modifiers applied by the game.
    [[nodiscard]] int bonus_damage() const { return m_tier[idx(UpgradeKind::Damage)]; }
    [[nodiscard]] f32 fire_rate_factor() const
    {
        return 1.0f + 0.15f * static_cast<f32>(m_tier[idx(UpgradeKind::FireRate)]);
    }
    [[nodiscard]] int bonus_health() const { return m_tier[idx(UpgradeKind::MaxHealth)]; }
    [[nodiscard]] f32 move_speed_mult() const
    {
        return 1.0f + 0.10f * static_cast<f32>(m_tier[idx(UpgradeKind::MoveSpeed)]);
    }

    void reset() { m_tier = {}; }

private:
    [[nodiscard]] static usize idx(UpgradeKind k) { return static_cast<usize>(k); }
    std::array<int, kUpgradeKinds> m_tier{};
};

} // namespace vyro::game
