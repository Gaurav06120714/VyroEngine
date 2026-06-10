# VyroEngine — Tests
# Lightweight assertion-based tests for the bootstrap phase. A full framework
# (Catch2 / GoogleTest) is integrated in the testing sub-phase.
add_executable(vyro_tests test_main.cpp)
target_link_libraries(vyro_tests PRIVATE vyro::engine vyro_warnings)

add_test(NAME vyro_core_tests COMMAND vyro_tests)
