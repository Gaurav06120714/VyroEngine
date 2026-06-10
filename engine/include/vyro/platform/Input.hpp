// VyroEngine — Input System
// Phase 1.5: tracks per-key/button state (held/pressed/released), mouse
// position and delta, scroll, and an action-mapping layer for rebindable
// logical actions. The platform backend feeds raw events; gameplay queries
// logical state. Decoupling raw input from this layer keeps it testable.
#pragma once

#include "vyro/platform/KeyCode.hpp"

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vyro {

class Input
{
public:
    Input() = default;

    // ── Frame lifecycle ──────────────────────────────────────────────
    // Clears per-frame transitions (pressed/released), scroll, and mouse
    // delta. Call once at the start of each frame before feeding events.
    void new_frame();

    // ── Raw event feed (called by the platform backend) ──────────────
    void on_key(KeyCode key, bool pressed);
    void on_mouse_button(MouseButton button, bool pressed);
    void on_mouse_move(f32 x, f32 y);
    void on_scroll(f32 delta);

    // ── Key queries ──────────────────────────────────────────────────
    [[nodiscard]] bool is_key_down(KeyCode key) const;     // held this frame
    [[nodiscard]] bool is_key_pressed(KeyCode key) const;  // went down this frame
    [[nodiscard]] bool is_key_released(KeyCode key) const; // went up this frame

    // ── Mouse queries ────────────────────────────────────────────────
    [[nodiscard]] bool is_button_down(MouseButton button) const;
    [[nodiscard]] bool is_button_pressed(MouseButton button) const;
    [[nodiscard]] bool is_button_released(MouseButton button) const;
    [[nodiscard]] f32 mouse_x() const { return m_mouse_x; }
    [[nodiscard]] f32 mouse_y() const { return m_mouse_y; }
    [[nodiscard]] f32 mouse_delta_x() const { return m_mouse_delta_x; }
    [[nodiscard]] f32 mouse_delta_y() const { return m_mouse_delta_y; }
    [[nodiscard]] f32 scroll_delta() const { return m_scroll; }

    // ── Action mapping ───────────────────────────────────────────────
    // Bind a logical action to a key; multiple keys may map to one action.
    void bind_action(std::string_view action, KeyCode key);
    void clear_action(std::string_view action);

    [[nodiscard]] bool is_action_down(std::string_view action) const;
    [[nodiscard]] bool is_action_pressed(std::string_view action) const;
    [[nodiscard]] bool is_action_released(std::string_view action) const;

private:
    std::array<bool, kKeyCount> m_key_down{};
    std::array<bool, kKeyCount> m_key_pressed{};
    std::array<bool, kKeyCount> m_key_released{};

    std::array<bool, kMouseButtonCount> m_btn_down{};
    std::array<bool, kMouseButtonCount> m_btn_pressed{};
    std::array<bool, kMouseButtonCount> m_btn_released{};

    f32 m_mouse_x = 0.0f;
    f32 m_mouse_y = 0.0f;
    f32 m_mouse_delta_x = 0.0f;
    f32 m_mouse_delta_y = 0.0f;
    f32 m_scroll = 0.0f;

    std::unordered_map<std::string, std::vector<KeyCode>> m_actions;
};

} // namespace vyro
