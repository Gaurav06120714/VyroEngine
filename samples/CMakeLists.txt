# VyroEngine — Samples

# Headless terminal demo (always built).
add_executable(bounce bounce/main.cpp)
target_link_libraries(bounce PRIVATE vyro::engine vyro_warnings)
set_target_properties(bounce PROPERTIES OUTPUT_NAME "vyro_bounce")

# Windowed OpenGL demo (only when the GL backend is available).
if(VYRO_GL_AVAILABLE)
    add_executable(window_demo window/main.cpp)
    target_link_libraries(window_demo PRIVATE vyro::gl vyro_warnings)
    set_target_properties(window_demo PROPERTIES OUTPUT_NAME "vyro_window")

    add_executable(model_demo model/main.cpp)
    target_link_libraries(model_demo PRIVATE vyro::gl vyro_warnings)
    set_target_properties(model_demo PROPERTIES OUTPUT_NAME "vyro_model")
    message(STATUS "VyroEngine: windowed demos (vyro_window, vyro_model) enabled")
endif()

# Vulkan on-screen demo (V2.4 stage b): needs Vulkan + GLFW + glslangValidator.
if(VYRO_VULKAN_AVAILABLE AND VYRO_GL_AVAILABLE)
    find_program(GLSLANG_VALIDATOR glslangValidator
        PATHS /opt/homebrew/bin /usr/local/bin)
    if(GLSLANG_VALIDATOR)
        set(VYRO_SHADER_DIR ${CMAKE_SOURCE_DIR}/assets/shaders)
        set(VYRO_SPV_OUT ${VYRO_SHADER_DIR})
        add_custom_command(
            OUTPUT ${VYRO_SPV_OUT}/model.vert.spv ${VYRO_SPV_OUT}/model.frag.spv
            COMMAND ${GLSLANG_VALIDATOR} -V ${VYRO_SHADER_DIR}/model.vert -o ${VYRO_SPV_OUT}/model.vert.spv
            COMMAND ${GLSLANG_VALIDATOR} -V ${VYRO_SHADER_DIR}/model.frag -o ${VYRO_SPV_OUT}/model.frag.spv
            DEPENDS ${VYRO_SHADER_DIR}/model.vert ${VYRO_SHADER_DIR}/model.frag
            COMMENT "Compiling GLSL to SPIR-V")
        add_custom_target(vyro_shaders DEPENDS
            ${VYRO_SPV_OUT}/model.vert.spv ${VYRO_SPV_OUT}/model.frag.spv)

        add_executable(vulkan_demo vulkan/main.cpp)
        add_dependencies(vulkan_demo vyro_shaders)
        target_include_directories(vulkan_demo PRIVATE ${VULKAN_INCLUDE_DIR})
        # vyro::gl carries the glfw imported target (scoped to engine/) transitively.
        target_link_libraries(vulkan_demo PRIVATE vyro::gl ${VULKAN_LIBRARY} vyro_warnings)
        set_target_properties(vulkan_demo PROPERTIES OUTPUT_NAME "vyro_vulkan")
        message(STATUS "VyroEngine: Vulkan demo (vyro_vulkan) enabled")
    else()
        message(WARNING "VyroEngine: glslangValidator not found; skipping Vulkan demo")
    endif()
endif()
