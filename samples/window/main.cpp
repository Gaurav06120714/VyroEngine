// VyroEngine — Windowed OpenGL demo
// Opens a real window and renders the bouncing-ball physics simulation with the
// OpenGL backend: the engine's Window, OpenGLDevice (RHI), and PhysicsWorld
// working together on screen.
#include "vyro/core/Engine.hpp"
#include "vyro/core/Log.hpp"
#include "vyro/physics/PhysicsWorld.hpp"
#include "vyro/platform/Window.hpp"
#include "vyro/render/OpenGLDevice.hpp"
#include "vyro/render/Renderer2D.hpp" // QuadVertex

#include <vector>

namespace {

constexpr vyro::f32 kWorldHeight = 12.0f;

// Push a colored quad (two triangles, 6 vertices) in NDC space.
void push_quad(std::vector<vyro::QuadVertex>& out, vyro::f32 cx, vyro::f32 cy,
               vyro::f32 hw, vyro::f32 hh, vyro::Vec4 color)
{
    const vyro::Vec3 bl{cx - hw, cy - hh, 0.0f};
    const vyro::Vec3 br{cx + hw, cy - hh, 0.0f};
    const vyro::Vec3 tr{cx + hw, cy + hh, 0.0f};
    const vyro::Vec3 tl{cx - hw, cy + hh, 0.0f};
    out.push_back({bl, color, {0, 0}});
    out.push_back({br, color, {1, 0}});
    out.push_back({tr, color, {1, 1}});
    out.push_back({bl, color, {0, 0}});
    out.push_back({tr, color, {1, 1}});
    out.push_back({tl, color, {0, 1}});
}

// World y in [0, kWorldHeight] -> NDC y in [-1, 1].
vyro::f32 to_ndc_y(vyro::f32 world_y)
{
    return (world_y / kWorldHeight) * 2.0f - 1.0f;
}

} // namespace

int main()
{
    vyro::Engine::print_banner();

    vyro::Window window(800, 600, "VyroEngine - Bouncing Ball (OpenGL)");
    if (!window.valid()) {
        VYRO_ERROR("Demo", "no window/display available; cannot open the application");
        return 1;
    }

    vyro::OpenGLDevice device;

    const char* vs = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUV;
out vec4 vColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
})";

    const char* fs = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; })";

    const vyro::ShaderHandle shader = device.create_shader({vs, fs});

    vyro::BufferDesc bd;
    bd.type = vyro::BufferType::Vertex;
    bd.size = sizeof(vyro::QuadVertex) * 64; // room for floor + ball
    const vyro::BufferHandle vbo = device.create_buffer(bd);

    // Physics: floor + bouncing ball.
    vyro::PhysicsWorld world;
    world.set_gravity(vyro::Vec3{0.0f, -9.81f, 0.0f});

    constexpr vyro::f32 floor_radius = 1000.0f;
    vyro::RigidBody floor;
    floor.position = vyro::Vec3{0.0f, -floor_radius, 0.0f};
    floor.radius = floor_radius;
    floor.restitution = 0.88f;
    floor.set_mass(0.0f);
    world.add_body(floor);

    vyro::RigidBody ball;
    ball.position = vyro::Vec3{0.0f, 11.0f, 0.0f};
    ball.radius = 0.4f;
    ball.restitution = 0.88f;
    ball.set_mass(1.0f);
    const vyro::BodyId ball_id = world.add_body(ball);

    VYRO_INFO("Demo", "Window open. Close it to quit.");

    std::vector<vyro::QuadVertex> verts;
    verts.reserve(64);

    while (window.is_open()) {
        world.step();
        world.step();

        const vyro::f32 by = to_ndc_y(world.body(ball_id).position.y);

        verts.clear();
        // Floor strip across the bottom.
        push_quad(verts, 0.0f, -0.95f, 1.0f, 0.05f, vyro::Vec4{0.20f, 0.70f, 0.35f, 1.0f});
        // The ball.
        push_quad(verts, 0.0f, by, 0.08f, 0.10f, vyro::Vec4{1.0f, 0.55f, 0.10f, 1.0f});

        device.set_viewport(window.width(), window.height());
        device.clear(vyro::Vec4{0.07f, 0.08f, 0.12f, 1.0f});
        device.update_buffer(vbo, verts.data(), verts.size() * sizeof(vyro::QuadVertex));

        vyro::DrawCommand cmd;
        cmd.shader = shader;
        cmd.vertex_buffer = vbo;
        cmd.vertex_count = static_cast<vyro::u32>(verts.size());
        cmd.topology = vyro::PrimitiveTopology::Triangles;
        device.draw(cmd);

        window.swap_buffers();
        window.poll_events();
    }

    VYRO_INFO("Demo", "Window closed. Goodbye.");
    return 0;
}
