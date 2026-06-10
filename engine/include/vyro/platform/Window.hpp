// VyroEngine — Window (GLFW)
// Real on-screen window with an OpenGL 3.3 core context. Only built when GLFW
// is available (VYRO_ENABLE_GL); the headless build does not require it.
#pragma once

#include "vyro/core/Types.hpp"

#include <string>

struct GLFWwindow;

namespace vyro {

class Window : NonCopyable
{
public:
    Window(u32 width, u32 height, std::string title);
    ~Window();

    [[nodiscard]] bool is_open() const;
    void poll_events();
    void swap_buffers();
    void close();

    [[nodiscard]] u32 width() const { return m_width; }
    [[nodiscard]] u32 height() const { return m_height; }
    [[nodiscard]] GLFWwindow* native() const { return m_window; }
    [[nodiscard]] bool valid() const { return m_window != nullptr; }

private:
    GLFWwindow* m_window = nullptr;
    u32 m_width = 0;
    u32 m_height = 0;
    std::string m_title;
};

} // namespace vyro
