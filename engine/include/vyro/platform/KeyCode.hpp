// VyroEngine — Input key and button codes
// Phase 1.5: engine-internal, platform-agnostic input identifiers. The
// platform backend translates native codes (SDL/GLFW) into these.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro {

// Contiguous 0..Count enumeration so states can be stored in flat arrays.
enum class KeyCode : u16 {
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    // Digits
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    // Control
    Space, Enter, Escape, Tab, Backspace, Delete,
    LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,
    // Arrows
    Up, Down, Left, Right,

    Count,
};

enum class MouseButton : u8 {
    Left,
    Right,
    Middle,

    Count,
};

[[nodiscard]] inline constexpr usize key_index(KeyCode key)
{
    return static_cast<usize>(key);
}

[[nodiscard]] inline constexpr usize button_index(MouseButton button)
{
    return static_cast<usize>(button);
}

inline constexpr usize kKeyCount = static_cast<usize>(KeyCode::Count);
inline constexpr usize kMouseButtonCount = static_cast<usize>(MouseButton::Count);

} // namespace vyro
