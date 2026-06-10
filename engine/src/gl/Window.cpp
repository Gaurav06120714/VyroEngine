// VyroEngine — Window (GLFW) implementation
#include "vyro/platform/Window.hpp"

#include "vyro/core/Log.hpp"

#include <GLFW/glfw3.h>

#include <utility>

namespace vyro {

namespace {
int g_window_count = 0;
} // namespace

Window::Window(u32 width, u32 height, std::string title)
    : m_width(width), m_height(height), m_title(std::move(title))
{
    if (g_window_count == 0) {
        if (glfwInit() != GLFW_TRUE) {
            VYRO_ERROR("Window", "glfwInit failed");
            return;
        }
    }

    // Request an OpenGL 3.3 core, forward-compatible context (required on macOS).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                                m_title.c_str(), nullptr, nullptr);
    if (m_window == nullptr) {
        VYRO_ERROR("Window", "failed to create GLFW window");
        return;
    }
    ++g_window_count;

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // vsync
    VYRO_INFO("Window", "opened {}x{} '{}'", width, height, m_title);
}

Window::~Window()
{
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        --g_window_count;
        if (g_window_count == 0) {
            glfwTerminate();
        }
    }
}

bool Window::is_open() const
{
    return m_window != nullptr && glfwWindowShouldClose(m_window) == 0;
}

void Window::poll_events()
{
    glfwPollEvents();
}

void Window::swap_buffers()
{
    if (m_window != nullptr) {
        glfwSwapBuffers(m_window);
    }
}

void Window::close()
{
    if (m_window != nullptr) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

} // namespace vyro
