# VyroEngine — Root build configuration
# Phase 1.2: Build System
cmake_minimum_required(VERSION 3.20)

project(VyroEngine
    VERSION 0.1.0
    DESCRIPTION "A modern cross-platform game engine built from scratch"
    LANGUAGES CXX)

# ---------------------------------------------------------------- standards
set(CMAKE_CXX_STANDARD 20)
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

if(VYRO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "VyroEngine ${PROJECT_VERSION} configured (${CMAKE_BUILD_TYPE})")
