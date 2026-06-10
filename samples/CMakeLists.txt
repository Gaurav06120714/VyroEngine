# VyroEngine — Samples

# Headless terminal demo (always built).
add_executable(bounce bounce/main.cpp)
target_link_libraries(bounce PRIVATE vyro::engine vyro_warnings)
set_target_properties(bounce PROPERTIES OUTPUT_NAME "vyro_bounce")

# Windowed OpenGL demo (only when the GL backend is available).
if(VYRO_GL_AVAILABLE)
    add_executable(window_demo window/main.cpp)
    target_link_libraries(window_demo PRIVATE vyro::gl vyro_warnings)
    set_target_properties(window_demo PROPERTIES OUTPUT_NAME "vyro_window")
    message(STATUS "VyroEngine: windowed demo (vyro_window) enabled")
endif()
