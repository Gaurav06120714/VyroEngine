# VyroEngine — Root build configuration
# Phase 1.2: Build System
cmake_minimum_required(VERSION 3.20)

project(VyroEngine
    VERSION 8.0.0
    DESCRIPTION "A modern cross-platform game engine built from scratch"
    LANGUAGES CXX)

# ---------------------------------------------------------------- standards
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Single output directories for all targets.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------------- options
option(VYRO_BUILD_TESTS   "Build VyroEngine unit tests" ON)
option(VYRO_BUILD_RUNTIME "Build the runtime executable" ON)
option(VYRO_BUILD_SAMPLES "Build VyroEngine sample programs" ON)
option(VYRO_ENABLE_GL     "Build the OpenGL + GLFW window backend" ON)
option(VYRO_ENABLE_LUA    "Build the Lua scripting backend" ON)
option(VYRO_ENABLE_VULKAN "Build the Vulkan backend (MoltenVK on macOS)" ON)
option(VYRO_ENABLE_AUDIO  "Build the miniaudio output device" ON)

# Help find_package locate Homebrew-installed dependencies (e.g. glfw3).
if(APPLE AND EXISTS "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/opt/homebrew/opt/glfw")
endif()
option(VYRO_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# ---------------------------------------------------------------- warnings
add_library(vyro_warnings INTERFACE)
if(MSVC)
    target_compile_options(vyro_warnings INTERFACE /W4)
    if(VYRO_WARNINGS_AS_ERRORS)
        target_compile_options(vyro_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(vyro_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    if(VYRO_WARNINGS_AS_ERRORS)
        target_compile_options(vyro_warnings INTERFACE -Werror)
    endif()
endif()

# ---------------------------------------------------------------- subdirs
add_subdirectory(engine)

if(VYRO_BUILD_RUNTIME)
    add_subdirectory(runtime)
endif()

if(VYRO_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

if(VYRO_IMGUI_AVAILABLE)
    add_subdirectory(editor)
endif()

if(VYRO_GL_AVAILABLE)
    add_subdirectory(games)
endif()

if(VYRO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------- packaging
# Phase 12.3: install rules + CPack source/binary packages.
include(GNUInstallDirs)
install(TARGETS vyro_engine
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY engine/include/vyro
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(VYRO_BUILD_RUNTIME)
    install(TARGETS vyro_runtime
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(FILES README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

set(CPACK_PACKAGE_NAME "VyroEngine")
set(CPACK_PACKAGE_VENDOR "VyroEcosystem")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/build/" "/\\\\.git/" "\\\\.DS_Store")
include(CPack)

message(STATUS "VyroEngine ${PROJECT_VERSION} configured (${CMAKE_BUILD_TYPE})")
