// VyroEngine — Input System implementation
#include "vyro/platform/Input.hpp"

#include <string>

namespace vyro {

void Input::new_frame()
{
    m_key_pressed.fill(false);
    m_key_released.fill(false);
    m_btn_pressed.fill(false);
    m_btn_released.fill(false);
    m_mouse_delta_x = 0.0f;
    m_mouse_delta_y = 0.0f;
    m_scroll = 0.0f;
}

void Input::on_key(KeyCode key, bool pressed)
{
    const usize i = key_index(key);
    if (pressed) {
        if (!m_key_down[i]) {
            m_key_pressed[i] = true;
        }
        m_key_down[i] = true;
    } else {
        if (m_key_down[i]) {
            m_key_released[i] = true;
        }
        m_key_down[i] = false;
    }
}

void Input::on_mouse_button(MouseButton button, bool pressed)
{
    const usize i = button_index(button);
    if (pressed) {
        if (!m_btn_down[i]) {
            m_btn_pressed[i] = true;
        }
        m_btn_down[i] = true;
    } else {
        if (m_btn_down[i]) {
            m_btn_released[i] = true;
        }
        m_btn_down[i] = false;
    }
}

void Input::on_mouse_move(f32 x, f32 y)
{
    m_mouse_delta_x += x - m_mouse_x;
    m_mouse_delta_y += y - m_mouse_y;
    m_mouse_x = x;
    m_mouse_y = y;
}

void Input::on_scroll(f32 delta)
{
    m_scroll += delta;
}

bool Input::is_key_down(KeyCode key) const { return m_key_down[key_index(key)]; }
bool Input::is_key_pressed(KeyCode key) const { return m_key_pressed[key_index(key)]; }
bool Input::is_key_released(KeyCode key) const { return m_key_released[key_index(key)]; }

bool Input::is_button_down(MouseButton button) const { return m_btn_down[button_index(button)]; }
bool Input::is_button_pressed(MouseButton button) const { return m_btn_pressed[button_index(button)]; }
bool Input::is_button_released(MouseButton button) const { return m_btn_released[button_index(button)]; }

void Input::bind_action(std::string_view action, KeyCode key)
{
    m_actions[std::string(action)].push_back(key);
}

void Input::clear_action(std::string_view action)
{
    m_actions.erase(std::string(action));
}

bool Input::is_action_down(std::string_view action) const
{
    const auto it = m_actions.find(std::string(action));
    if (it == m_actions.end()) {
        return false;
    }
    for (const KeyCode key : it->second) {
        if (m_key_down[key_index(key)]) {
            return true;
        }
    }
    return false;
}

bool Input::is_action_pressed(std::string_view action) const
{
    const auto it = m_actions.find(std::string(action));
    if (it == m_actions.end()) {
        return false;
    }
    for (const KeyCode key : it->second) {
        if (m_key_pressed[key_index(key)]) {
            return true;
        }
    }
    return false;
}

bool Input::is_action_released(std::string_view action) const
{
    const auto it = m_actions.find(std::string(action));
    if (it == m_actions.end()) {
        return false;
    }
    for (const KeyCode key : it->second) {
        if (m_key_released[key_index(key)]) {
            return true;
        }
    }
    return false;
}

} // namespace vyro
