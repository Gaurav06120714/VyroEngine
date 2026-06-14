// VyroEngine — Player upgrade tests (V9.2)
#include "vyro/game/Upgrades.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("upgrades");

    game::Upgrades up;
    game::Economy econ;

    // Defaults: no bonuses, neutral multipliers.
    suite.check(up.tier(game::UpgradeKind::Damage) == 0, "starts at tier 0");
    suite.check(up.bonus_damage() == 0 && approx(up.fire_rate_factor(), 1.0f),
                "no modifiers at tier 0");
    suite.check(approx(up.move_speed_mult(), 1.0f), "neutral move speed at tier 0");

    // Can't buy with no credits.
    suite.check(!up.buy(econ, game::UpgradeKind::Damage), "cannot buy when broke");

    // Buy raises the tier and spends; cost rises with each tier.
    econ.earn(1000);
    const int c0 = up.cost(game::UpgradeKind::Damage);
    suite.check(up.buy(econ, game::UpgradeKind::Damage), "buy succeeds with credits");
    suite.check(up.tier(game::UpgradeKind::Damage) == 1 && up.bonus_damage() == 1,
                "tier and bonus increased");
    suite.check(up.cost(game::UpgradeKind::Damage) > c0, "next tier costs more");
    suite.check(econ.credits() == 1000 - c0, "credits spent");

    // Modifiers scale with tier.
    up.buy(econ, game::UpgradeKind::FireRate);
    suite.check(up.fire_rate_factor() > 1.0f, "fire-rate factor rises");
    up.buy(econ, game::UpgradeKind::MoveSpeed);
    suite.check(up.move_speed_mult() > 1.0f, "move speed rises");

    // Cap at max tier.
    {
        game::Upgrades capped;
        game::Economy rich;
        rich.earn(100000);
        for (int i = 0; i < 20; ++i) {
            capped.buy(rich, game::UpgradeKind::MaxHealth);
        }
        suite.check(capped.tier(game::UpgradeKind::MaxHealth) == game::kMaxTier, "tier caps");
        suite.check(!capped.buy(rich, game::UpgradeKind::MaxHealth), "cannot exceed the cap");
        suite.check(capped.bonus_health() == game::kMaxTier, "bonus health at cap");
    }

    up.reset();
    suite.check(up.tier(game::UpgradeKind::FireRate) == 0, "reset clears tiers");

    return suite.summary();
}
