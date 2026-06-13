# VyroEngine — Core engine library (headless; no GPU/window dependencies)
file(GLOB_RECURSE VYRO_ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Backend sources (GL window, Lua) are built in their optional targets.
list(FILTER VYRO_ENGINE_SOURCES EXCLUDE REGEX "/src/(gl|lua|vk|ma)/")

add_library(vyro_engine STATIC ${VYRO_ENGINE_SOURCES})
add_library(vyro::engine ALIAS vyro_engine)

target_include_directories(vyro_engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(vyro_engine SYSTEM
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

target_link_libraries(vyro_engine PRIVATE vyro_warnings)

set_target_properties(vyro_engine PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

# ── Optional Lua scripting backend (vyro_lua) ────────────────────────
if(VYRO_ENABLE_LUA)
    find_path(LUA_INCLUDE_DIR lua.hpp PATH_SUFFIXES lua
        PATHS /opt/homebrew/opt/lua/include /usr/local/include /usr/include)
    find_library(LUA_LIBRARY NAMES lua liblua
        PATHS /opt/homebrew/opt/lua/lib /usr/local/lib /usr/lib)

    if(LUA_INCLUDE_DIR AND LUA_LIBRARY)
        add_library(vyro_lua STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src/lua/LuaScriptEngine.cpp)
        add_library(vyro::lua ALIAS vyro_lua)
        target_include_directories(vyro_lua
            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
            PRIVATE ${LUA_INCLUDE_DIR})
        target_link_libraries(vyro_lua PUBLIC vyro::engine PRIVATE ${LUA_LIBRARY})
        set(VYRO_LUA_AVAILABLE TRUE PARENT_SCOPE)
        message(STATUS "VyroEngine: Lua scripting backend (vyro_lua) enabled — ${LUA_LIBRARY}")
    else()
        message(WARNING "VyroEngine: VYRO_ENABLE_LUA set but Lua not found; skipping vyro_lua")
    endif()
endif()

# ── Optional real audio output (vyro_audio) via miniaudio ────────────
if(VYRO_ENABLE_AUDIO)
    add_library(vyro_audio STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ma/AudioDevice.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ma/AudioFile.cpp)
    add_library(vyro::audio ALIAS vyro_audio)
    target_include_directories(vyro_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_include_directories(vyro_audio SYSTEM
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio)
    target_link_libraries(vyro_audio PUBLIC vyro::engine)
    if(APPLE)
        target_link_libraries(vyro_audio PRIVATE
            "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
    endif()
    set(VYRO_AUDIO_AVAILABLE TRUE PARENT_SCOPE)
    message(STATUS "VyroEngine: audio output (vyro_audio) enabled")
endif()

# ── Optional Vulkan backend (vyro_vk) via MoltenVK ───────────────────
if(VYRO_ENABLE_VULKAN)
    find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h
        PATHS /opt/homebrew/opt/vulkan-headers/include /usr/local/include /usr/include)
    find_library(VULKAN_LIBRARY NAMES vulkan
        PATHS /opt/homebrew/opt/vulkan-loader/lib /usr/local/lib /usr/lib)

    if(VULKAN_INCLUDE_DIR AND VULKAN_LIBRARY)
        add_library(vyro_vk STATIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src/vk/VulkanDevice.cpp)
        add_library(vyro::vk ALIAS vyro_vk)
        target_include_directories(vyro_vk
            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
            PRIVATE ${VULKAN_INCLUDE_DIR})
        target_link_libraries(vyro_vk PUBLIC vyro::engine PRIVATE ${VULKAN_LIBRARY})
        set(VYRO_VULKAN_AVAILABLE TRUE PARENT_SCOPE)
        message(STATUS "VyroEngine: Vulkan backend (vyro_vk) enabled — ${VULKAN_LIBRARY}")
    else()
        message(WARNING "VyroEngine: VYRO_ENABLE_VULKAN set but Vulkan not found; skipping vyro_vk")
    endif()
endif()

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

        # ── Dear ImGui (editor UI), fetched at configure time ────────
        include(FetchContent)
        FetchContent_Declare(imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG v1.91.5
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(imgui)

        add_library(vyro_imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
        target_include_directories(vyro_imgui PUBLIC
            ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
        target_link_libraries(vyro_imgui PUBLIC glfw ${OPENGL_FRAMEWORK})
        set(VYRO_IMGUI_AVAILABLE TRUE PARENT_SCOPE)
        message(STATUS "VyroEngine: ImGui editor UI enabled")
    else()
        message(WARNING "VyroEngine: VYRO_ENABLE_GL set but glfw3/OpenGL not found; skipping vyro_gl")
    endif()
endif()
