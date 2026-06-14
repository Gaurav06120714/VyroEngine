// VyroEngine — Environmental hazard tests (V10.1)
#include "vyro/game/Hazard.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("hazard");

    game::Hazard h;
    h.center = Vec3{5, 0, 5};
    h.radius = 2.0f;

    // Containment ignores height.
    suite.check(game::hazard_contains(h, Vec3{5, 3, 5}), "point above the center is inside");
    suite.check(game::hazard_contains(h, Vec3{6.5f, 0, 5}), "within radius is inside");
    suite.check(!game::hazard_contains(h, Vec3{9, 0, 5}), "beyond radius is outside");

    // Expiry.
    suite.check(!game::hazard_expired(h), "fresh hazard is alive");
    h.life = 0.0f;
    suite.check(game::hazard_expired(h), "depleted hazard is expired");

    // Damage accrual: 4 dps over 0.25s = 1.0 -> one whole hit, no remainder.
    {
        f32 carry = 0.0f;
        suite.check(game::accrue_damage(4.0f, 0.25f, carry) == 1 && approx(carry, 0.0f),
                    "exact tick yields one hit");
    }
    // 4 dps over 0.1s = 0.4 -> 0 now, carried; three more reach 1.
    {
        f32 carry = 0.0f;
        suite.check(game::accrue_damage(4.0f, 0.1f, carry) == 0, "fractional damage carries");
        (void)game::accrue_damage(4.0f, 0.1f, carry);
        const int hit = game::accrue_damage(4.0f, 0.1f, carry); // total 1.2
        suite.check(hit == 1, "accumulated fractions become a hit");
    }
    // Zero dps does nothing.
    {
        f32 carry = 0.0f;
        suite.check(game::accrue_damage(0.0f, 1.0f, carry) == 0, "no dps, no damage");
    }

    return suite.summary();
}
