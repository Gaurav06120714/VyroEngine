// VyroEngine — Input tests
#include "vyro/platform/Input.hpp"

#include "test_harness.hpp"

int main()
{
    vyro::test::Suite suite("input");

    // Input_KeyPress_SetsPressedAndDownForOneFrame
    {
        vyro::Input input;
        input.new_frame();
        input.on_key(vyro::KeyCode::Space, true);
        suite.check(input.is_key_down(vyro::KeyCode::Space), "Space down after press");
        suite.check(input.is_key_pressed(vyro::KeyCode::Space), "Space pressed on the press frame");

        input.new_frame();
        suite.check(input.is_key_down(vyro::KeyCode::Space), "Space still held next frame");
        suite.check(!input.is_key_pressed(vyro::KeyCode::Space), "pressed cleared next frame");
    }

    // Input_KeyRelease_SetsReleasedForOneFrame
    {
        vyro::Input input;
        input.new_frame();
        input.on_key(vyro::KeyCode::W, true);
        input.new_frame();
        input.on_key(vyro::KeyCode::W, false);
        suite.check(input.is_key_released(vyro::KeyCode::W), "W released on release frame");
        suite.check(!input.is_key_down(vyro::KeyCode::W), "W no longer down");
    }

    // Input_MouseMove_AccumulatesDelta
    {
        vyro::Input input;
        input.new_frame();
        input.on_mouse_move(100.0f, 50.0f);
        input.on_mouse_move(110.0f, 50.0f);
        suite.check(input.mouse_x() == 110.0f, "mouse x tracks latest position");
        suite.check(input.mouse_delta_x() == 110.0f, "delta x accumulates from origin");
        input.new_frame();
        suite.check(input.mouse_delta_x() == 0.0f, "delta resets each frame");
    }

    // Input_Action_MapsBoundKeys
    {
        vyro::Input input;
        input.bind_action("Jump", vyro::KeyCode::Space);
        input.bind_action("Jump", vyro::KeyCode::W);
        input.new_frame();
        input.on_key(vyro::KeyCode::W, true);
        suite.check(input.is_action_pressed("Jump"), "Jump pressed via bound W");
        suite.check(input.is_action_down("Jump"), "Jump down via bound W");
        suite.check(!input.is_action_down("Fire"), "unbound action is inactive");
    }

    return suite.summary();
}
