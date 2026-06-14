// VyroEngine — Weapon unlock tests (V10.3)
#include "vyro/game/WeaponUnlock.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("weapon_unlock");

    // Pistol (0) is always available.
    suite.check(game::weapon_unlocked(0, 1), "pistol unlocked at wave 1");

    // Rifle (1) unlocks at wave 2.
    suite.check(!game::weapon_unlocked(1, 1), "rifle locked at wave 1");
    suite.check(game::weapon_unlocked(1, 2), "rifle unlocked at wave 2");

    // Shotgun (2) unlocks at wave 3.
    suite.check(!game::weapon_unlocked(2, 2), "shotgun locked at wave 2");
    suite.check(game::weapon_unlocked(2, 5), "shotgun unlocked by wave 5");

    // Out-of-range indices are never unlocked.
    suite.check(!game::weapon_unlocked(9, 99), "unknown weapon stays locked");

    // Unlock count grows with the wave.
    suite.check(game::unlocked_count(1) == 1, "one weapon at wave 1");
    suite.check(game::unlocked_count(2) == 2, "two weapons at wave 2");
    suite.check(game::unlocked_count(3) == 3, "all three by wave 3");

    return suite.summary();
}
