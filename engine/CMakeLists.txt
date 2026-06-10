# VyroEngine — Core engine library
file(GLOB_RECURSE VYRO_ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(vyro_engine STATIC ${VYRO_ENGINE_SOURCES})
add_library(vyro::engine ALIAS vyro_engine)

target_include_directories(vyro_engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(vyro_engine PRIVATE vyro_warnings)

set_target_properties(vyro_engine PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
