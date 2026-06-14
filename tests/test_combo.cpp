// VyroEngine — Combo multiplier tests (V9.4)
#include "vyro/game/Combo.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("combo");

    // window 2s, 3 kills per tier, cap 5.
    game::Combo c(2.0f, 3, 5);
    suite.check(c.count() == 0 && c.multiplier() == 1, "starts at 1x");

    c.on_kill();
    c.on_kill();
    suite.check(c.multiplier() == 1, "two kills still 1x");
    c.on_kill();
    suite.check(c.count() == 3 && c.multiplier() == 2, "three kills -> 2x");

    for (int i = 0; i < 3; ++i) {
        c.on_kill();
    }
    suite.check(c.multiplier() == 3, "six kills -> 3x");

    // Within the window the streak holds; past it, it breaks.
    c.update(1.0f);
    suite.check(c.count() == 6, "streak holds within the window");
    c.update(1.5f); // total 2.5s since last kill > 2s window
    suite.check(c.count() == 0 && c.multiplier() == 1, "streak decays after the window");

    // Multiplier is capped.
    {
        game::Combo big(5.0f, 1, 4); // +1 per kill, cap 4
        for (int i = 0; i < 20; ++i) {
            big.on_kill();
        }
        suite.check(big.multiplier() == 4, "multiplier caps");
    }

    c.reset();
    suite.check(c.count() == 0, "reset clears the streak");

    return suite.summary();
}
