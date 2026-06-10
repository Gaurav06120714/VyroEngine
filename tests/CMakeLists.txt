# VyroEngine — Tests
# Lightweight assertion-based tests for the bootstrap phase. A full framework
# (GoogleTest) is integrated in the testing sub-phase per rulz/TESTING_RULES.

# Each test_*.cpp becomes its own executable + ctest case.
file(GLOB VYRO_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)

foreach(test_src ${VYRO_TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${test_name} PRIVATE vyro::engine vyro_warnings)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
