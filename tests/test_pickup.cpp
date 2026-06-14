// VyroEngine — Pickup tests (V8.2)
#include "vyro/game/Pickup.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("pickup");

    // Drop gate.
    suite.check(game::should_drop(0.1f, 0.25f), "roll below chance drops");
    suite.check(!game::should_drop(0.3f, 0.25f), "roll above chance does not drop");
    suite.check(!game::should_drop(0.5f, 0.0f), "zero chance never drops");

    // Weighted kind: health 30 / ammo 50 / score 20 (total 100).
    {
        const game::PickupTable t; // defaults
        suite.check(game::pick_kind(t, 0.1f) == game::PickupKind::Health, "low roll -> health");
        suite.check(game::pick_kind(t, 0.5f) == game::PickupKind::Ammo, "mid roll -> ammo");
        suite.check(game::pick_kind(t, 0.95f) == game::PickupKind::ScoreBoost,
                    "high roll -> score boost");
    }

    // Effect amounts are sane.
    suite.check(game::kHealthRestore >= 1, "health restores at least one heart");
    suite.check(game::kScoreBoost > 0, "score boost is positive");

    return suite.summary();
}
