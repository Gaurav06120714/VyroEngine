# VyroEngine — Core engine library (headless; no GPU/window dependencies)
file(GLOB_RECURSE VYRO_ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# GL/window backend sources are built separately in the optional vyro_gl target.
list(FILTER VYRO_ENGINE_SOURCES EXCLUDE REGEX "/src/gl/")

add_library(vyro_engine STATIC ${VYRO_ENGINE_SOURCES})
add_library(vyro::engine ALIAS vyro_engine)

target_include_directories(vyro_engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(vyro_engine PRIVATE vyro_warnings)

set_target_properties(vyro_engine PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

# ── Optional OpenGL + GLFW backend (vyro_gl) ─────────────────────────
if(VYRO_ENABLE_GL)
    find_package(glfw3 QUIET)
    find_library(OPENGL_FRAMEWORK OpenGL)

    if(glfw3_FOUND AND OPENGL_FRAMEWORK)
        add_library(vyro_gl STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src/gl/Window.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/gl/OpenGLDevice.cpp)
        add_library(vyro::gl ALIAS vyro_gl)
        target_include_directories(vyro_gl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_link_libraries(vyro_gl PUBLIC vyro::engine glfw ${OPENGL_FRAMEWORK})
        set(VYRO_GL_AVAILABLE TRUE PARENT_SCOPE)
        message(STATUS "VyroEngine: OpenGL backend (vyro_gl) enabled")
    else()
        message(WARNING "VyroEngine: VYRO_ENABLE_GL set but glfw3/OpenGL not found; skipping vyro_gl")
    endif()
endif()
