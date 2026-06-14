// VyroEngine — Title / menu state (V10.5)
// A tiny front-end state machine: the game opens on a title screen and starts
// the run on demand, returning to the title later. Headless and tested.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro::game {

enum class Screen : u8 {
    Title = 0,
    Playing = 1,
};

class MenuState
{
public:
    [[nodiscard]] Screen screen() const { return m_screen; }
    [[nodiscard]] bool at_title() const { return m_screen == Screen::Title; }
    [[nodiscard]] bool playing() const { return m_screen == Screen::Playing; }

    void start() { m_screen = Screen::Playing; }
    void to_title() { m_screen = Screen::Title; }

private:
    Screen m_screen = Screen::Title;
};

} // namespace vyro::game
