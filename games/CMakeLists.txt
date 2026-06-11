# VyroEngine — Games built on the engine
if(VYRO_GL_AVAILABLE)
    add_executable(vyrostrike vyrostrike/main.cpp)
    target_link_libraries(vyrostrike PRIVATE vyro::gl vyro_warnings)
    set_target_properties(vyrostrike PROPERTIES OUTPUT_NAME "VyroStrike")
    message(STATUS "VyroEngine: game VyroStrike enabled")
endif()
