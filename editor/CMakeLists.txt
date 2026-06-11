# VyroEditor — visual editor application (V2.1)
add_executable(vyro_editor src/main.cpp)
target_link_libraries(vyro_editor PRIVATE vyro::gl vyro_imgui vyro_warnings)
set_target_properties(vyro_editor PROPERTIES OUTPUT_NAME "VyroEditor")
