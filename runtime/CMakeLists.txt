# VyroEngine — Runtime executable
add_executable(vyro_runtime src/main.cpp)

target_link_libraries(vyro_runtime
    PRIVATE vyro::engine vyro_warnings)

set_target_properties(vyro_runtime PROPERTIES
    OUTPUT_NAME "VyroEngine")
