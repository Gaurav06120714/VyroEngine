// VyroEngine — Economy tests (V9.1)
#include "vyro/game/Economy.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("economy");

    game::Economy e;
    suite.check(e.credits() == 0, "starts empty");

    e.earn(50);
    e.earn(25);
    suite.check(e.credits() == 75, "earning accumulates");
    e.earn(-10);
    suite.check(e.credits() == 75, "negative earn ignored");

    suite.check(e.can_afford(75) && !e.can_afford(76), "affordability is exact");
    suite.check(e.spend(30), "spend within balance succeeds");
    suite.check(e.credits() == 45, "balance reduced by spend");
    suite.check(!e.spend(100), "spend beyond balance fails");
    suite.check(e.credits() == 45, "failed spend leaves balance untouched");

    e.reset();
    suite.check(e.credits() == 0, "reset clears the wallet");

    return suite.summary();
}
