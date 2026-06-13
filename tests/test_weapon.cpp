// VyroEngine — Weapon model tests (V7.3)
// Spread lays pellets across the cone; the runtime gates firing by cooldown,
// ammo and reload.
#include "vyro/game/Weapon.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("weapon");

    // Spread: one pellet is centered; many are symmetric across the cone.
    {
        suite.check(game::spread_angles(1, 0.5f).size() == 1, "single pellet count");
        suite.check(approx(game::spread_angles(1, 0.5f)[0], 0.0f), "single pellet is centered");
        const auto s = game::spread_angles(5, 0.4f);
        suite.check(s.size() == 5, "five pellets");
        suite.check(approx(s.front(), -0.2f) && approx(s.back(), 0.2f), "spans the cone");
        suite.check(approx(s.front(), -s.back()), "symmetric about center");
    }

    // Firing consumes ammo and respects the cooldown.
    {
        game::Weapon w(game::WeaponStats{"rifle", 0.1f, 1, 0.0f, 1, 3, 1.0f});
        suite.check(w.can_fire(), "starts ready");
        suite.check(w.fire(), "first shot fires");
        suite.check(w.ammo() == 2, "ammo decremented");
        suite.check(!w.can_fire(), "cooldown blocks immediate refire");
        w.update(0.05f);
        suite.check(!w.can_fire(), "still cooling down");
        w.update(0.06f);
        suite.check(w.can_fire(), "ready after the interval");
    }

    // Empty magazine can't fire until reloaded.
    {
        game::Weapon w(game::WeaponStats{"pistol", 0.0f, 1, 0.0f, 1, 2, 0.5f});
        suite.check(w.fire() && w.fire(), "two shots empty the mag");
        suite.check(!w.fire(), "cannot fire when empty");
        w.begin_reload();
        suite.check(w.reloading(), "reload started");
        w.update(0.5f);
        suite.check(!w.reloading() && w.ammo() == 2, "mag refilled after reload");
        suite.check(w.can_fire(), "ready again after reload");
    }

    return suite.summary();
}
