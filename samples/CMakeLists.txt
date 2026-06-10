# VyroEngine — Samples
add_executable(bounce bounce/main.cpp)
target_link_libraries(bounce PRIVATE vyro::engine vyro_warnings)
set_target_properties(bounce PROPERTIES OUTPUT_NAME "vyro_bounce")
