// VyroEngine — Menu state tests (V10.5)
#include "vyro/game/MenuState.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("menu");

    game::MenuState m;
    suite.check(m.at_title() && !m.playing(), "opens on the title screen");

    m.start();
    suite.check(m.playing() && !m.at_title(), "start enters play");
    suite.check(m.screen() == game::Screen::Playing, "screen reports playing");

    m.to_title();
    suite.check(m.at_title(), "can return to the title");

    return suite.summary();
}
