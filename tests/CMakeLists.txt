# VyroEngine — Tests
# Lightweight assertion-based tests for the bootstrap phase. A full framework
# (GoogleTest) is integrated in the testing sub-phase per rulz/TESTING_RULES.

# Each test_*.cpp becomes its own executable + ctest case. Backend-specific
# tests (Lua) are handled separately so the core suite stays dependency-free.
file(GLOB VYRO_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)
list(REMOVE_ITEM VYRO_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_lua.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_vulkan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_audio_device.cpp)

foreach(test_src ${VYRO_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${test_name} PRIVATE vyro::engine vyro_warnings)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

if(VYRO_LUA_AVAILABLE)
    add_executable(test_lua test_lua.cpp)
    target_include_directories(test_lua PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_lua PRIVATE vyro::lua vyro_warnings)
    add_test(NAME test_lua COMMAND test_lua)
endif()

if(VYRO_VULKAN_AVAILABLE)
    add_executable(test_vulkan test_vulkan.cpp)
    target_include_directories(test_vulkan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_vulkan PRIVATE vyro::vk vyro_warnings)
    add_test(NAME test_vulkan COMMAND test_vulkan)
endif()

if(VYRO_AUDIO_AVAILABLE)
    add_executable(test_audio_device test_audio_device.cpp)
    target_include_directories(test_audio_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_audio_device PRIVATE vyro::audio vyro_warnings)
    add_test(NAME test_audio_device COMMAND test_audio_device)
endif()
