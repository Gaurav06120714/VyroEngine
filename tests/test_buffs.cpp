// VyroEngine — Timed buff tests (V10.4)
#include "vyro/game/Buffs.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-3f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("buffs");

    game::Buffs b;
    suite.check(!b.active(game::BuffKind::RapidFire), "no buffs at start");
    suite.check(approx(b.fire_rate_mult(), 1.0f) && b.damage_mult() == 1, "neutral mults");

    // Grant rapid fire.
    b.grant(game::BuffKind::RapidFire, 5.0f);
    suite.check(b.active(game::BuffKind::RapidFire), "rapid fire active");
    suite.check(approx(b.fire_rate_mult(), 2.0f), "rapid fire doubles fire rate");
    suite.check(b.damage_mult() == 1, "damage unaffected by rapid fire");

    // Decay.
    b.update(2.0f);
    suite.check(approx(b.remaining(game::BuffKind::RapidFire), 3.0f), "buff time decays");
    b.update(3.0f);
    suite.check(!b.active(game::BuffKind::RapidFire), "buff expires");
    suite.check(approx(b.fire_rate_mult(), 1.0f), "mult returns to neutral");

    // Double damage independent.
    b.grant(game::BuffKind::DoubleDamage, 4.0f);
    suite.check(b.damage_mult() == 2 && approx(b.fire_rate_mult(), 1.0f),
                "double damage is independent");

    // Grant refreshes to the longer duration only.
    b.grant(game::BuffKind::DoubleDamage, 1.0f); // shorter, ignored
    suite.check(b.remaining(game::BuffKind::DoubleDamage) > 3.0f, "shorter grant does not shorten");

    b.reset();
    suite.check(!b.active(game::BuffKind::DoubleDamage), "reset clears buffs");

    return suite.summary();
}
