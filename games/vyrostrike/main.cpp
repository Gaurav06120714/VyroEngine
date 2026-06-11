// VyroStrike — the first game built on VyroEngine.
// Fly the spaceship over the grid, dodge the incoming red raiders, and shoot
// them down. Built entirely from engine systems:
//   ECS Registry/View   — players, bullets, and enemies are entities
//   Input               — rebindable action map fed from the window's keys
//   Physics collision   — sphere-sphere hit tests
//   OBJ/MTL assets      — the spaceship model
//   OpenGL RHI          — textured, lit 3D rendering
//
// Controls:  A/D or Left/Right = move   Space = shoot   R = restart   Esc = quit
#include "vyro/assets/Image.hpp"
#include "vyro/assets/Mesh.hpp"
#include "vyro/assets/ObjLoader.hpp"
#include "vyro/core/Engine.hpp"
#include "vyro/core/Log.hpp"
#include "vyro/ecs/Registry.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/physics/Collision.hpp"
#include "vyro/platform/Input.hpp"
#include "vyro/platform/Window.hpp"
#include "vyro/render/Camera.hpp"
#include "vyro/render/OpenGLDevice.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <format>
#include <random>
#include <string>
#include <vector>

namespace {

// ── Components ───────────────────────────────────────────────────────
struct Position {
    vyro::Vec3 value{};
};
struct Velocity {
    vyro::Vec3 value{};
};
struct BulletTag {};
struct EnemyTag {};

// ── Tuning ───────────────────────────────────────────────────────────
constexpr vyro::f32 kArenaHalfWidth = 5.0f;
constexpr vyro::f32 kPlayerZ = 4.0f;
constexpr vyro::f32 kPlayerSpeed = 8.0f;
constexpr vyro::f32 kBulletSpeed = 18.0f;
constexpr vyro::f32 kFireCooldown = 0.22f;
constexpr vyro::f32 kSpawnZ = -28.0f;
constexpr vyro::f32 kEnemyRadius = 0.55f;
constexpr vyro::f32 kBulletRadius = 0.18f;
constexpr vyro::f32 kPlayerRadius = 0.6f;

struct GameState {
    vyro::f32 player_x = 0.0f;
    vyro::f32 fire_timer = 0.0f;
    vyro::f32 spawn_timer = 0.0f;
    vyro::f32 spawn_interval = 1.4f;
    vyro::f32 enemy_speed = 5.0f;
    int score = 0;
    bool game_over = false;
};

// Feed the window's physical keys into the engine's action-mapped Input.
void pump_input(vyro::Input& input, GLFWwindow* window)
{
    input.new_frame();
    auto feed = [&](int glfw_key, vyro::KeyCode code) {
        input.on_key(code, glfwGetKey(window, glfw_key) == GLFW_PRESS);
    };
    feed(GLFW_KEY_A, vyro::KeyCode::A);
    feed(GLFW_KEY_D, vyro::KeyCode::D);
    feed(GLFW_KEY_LEFT, vyro::KeyCode::Left);
    feed(GLFW_KEY_RIGHT, vyro::KeyCode::Right);
    feed(GLFW_KEY_SPACE, vyro::KeyCode::Space);
    feed(GLFW_KEY_R, vyro::KeyCode::R);
    feed(GLFW_KEY_ESCAPE, vyro::KeyCode::Escape);
}

vyro::MeshData load_ship()
{
    for (const char* p : {"assets/models/model.obj", "../assets/models/model.obj"}) {
        if (auto r = vyro::load_obj(p); r.has_value()) {
            return std::move(r.value());
        }
    }
    return vyro::make_cube(1.0f);
}

vyro::Mat4 fit_transform(const vyro::MeshData& mesh, vyro::f32 target)
{
    vyro::Vec3 lo = mesh.vertices[0].position;
    vyro::Vec3 hi = lo;
    for (const auto& v : mesh.vertices) {
        lo.x = std::min(lo.x, v.position.x);
        lo.y = std::min(lo.y, v.position.y);
        lo.z = std::min(lo.z, v.position.z);
        hi.x = std::max(hi.x, v.position.x);
        hi.y = std::max(hi.y, v.position.y);
        hi.z = std::max(hi.z, v.position.z);
    }
    const vyro::Vec3 c{(lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f, (lo.z + hi.z) * 0.5f};
    const vyro::f32 extent = std::max({hi.x - lo.x, hi.y - lo.y, hi.z - lo.z});
    const vyro::f32 s = extent > 0.0f ? target / extent : 1.0f;
    return vyro::Mat4::scale({s, s, s}) * vyro::Mat4::translation(-c);
}

struct GpuMesh {
    vyro::BufferHandle vbo;
    vyro::BufferHandle ibo;
    vyro::u32 index_count = 0;
};

GpuMesh upload(vyro::OpenGLDevice& device, const vyro::MeshData& mesh)
{
    GpuMesh out;
    out.vbo = device.create_buffer(
        {vyro::BufferType::Vertex, mesh.vertex_bytes(), mesh.vertices.data()});
    out.ibo = device.create_buffer(
        {vyro::BufferType::Index, mesh.index_bytes(), mesh.indices.data()});
    out.index_count = static_cast<vyro::u32>(mesh.indices.size());
    return out;
}

// Tint a mesh's vertex colors (procedural cubes default to white).
vyro::MeshData tinted_cube(vyro::f32 size, vyro::Vec3 color)
{
    vyro::MeshData cube = vyro::make_cube(size);
    for (auto& v : cube.vertices) {
        v.color = color;
    }
    return cube;
}

} // namespace

int main()
{
    vyro::Engine::print_banner();
    VYRO_INFO("Game", "VyroStrike — A/D move, Space shoot, R restart, Esc quit");

    vyro::Window window(1100, 760, "VyroStrike — score 0");
    if (!window.valid()) {
        return 1;
    }
    vyro::OpenGLDevice device;

    // ── Engine input with a rebindable action map ────────────────────
    vyro::Input input;
    input.bind_action("MoveLeft", vyro::KeyCode::A);
    input.bind_action("MoveLeft", vyro::KeyCode::Left);
    input.bind_action("MoveRight", vyro::KeyCode::D);
    input.bind_action("MoveRight", vyro::KeyCode::Right);
    input.bind_action("Fire", vyro::KeyCode::Space);
    input.bind_action("Restart", vyro::KeyCode::R);
    input.bind_action("Quit", vyro::KeyCode::Escape);

    // ── Shader (same textured/lit pipeline as the viewer) ────────────
    const char* vs = R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;  layout(location=3) in vec3 aColor;
uniform mat4 u_mvp; uniform mat4 u_model;
out vec3 vNormal; out vec3 vColor; out vec2 vUV;
void main(){ gl_Position=u_mvp*vec4(aPos,1.0); vNormal=mat3(u_model)*aNormal;
vColor=aColor; vUV=aUV; })";
    const char* fs = R"(#version 330 core
in vec3 vNormal; in vec3 vColor; in vec2 vUV; out vec4 FragColor;
uniform vec3 u_lightDir; uniform sampler2D u_texture;
void main(){ float d=max(dot(normalize(vNormal),normalize(-u_lightDir)),0.0);
vec3 base=texture(u_texture,vUV).rgb*vColor;
FragColor=vec4(base*(0.35+0.65*d),1.0); })";
    const auto shader = device.create_shader({vs, fs});

    // ── Assets ───────────────────────────────────────────────────────
    const vyro::MeshData ship_mesh = load_ship();
    const GpuMesh ship = upload(device, ship_mesh);
    const vyro::Mat4 ship_fit = fit_transform(ship_mesh, 1.6f);

    const GpuMesh enemy = upload(device, tinted_cube(1.0f, {0.95f, 0.25f, 0.2f}));
    const GpuMesh bullet = upload(device, tinted_cube(0.28f, {1.0f, 0.9f, 0.3f}));

    const vyro::Image checker = vyro::make_checkerboard(256, 32, 70, 75, 95, 35, 38, 52);
    const auto checker_tex = device.create_texture(
        {checker.width, checker.height, vyro::TextureFormat::RGBA8, checker.pixels.data()});
    const vyro::Image white = vyro::make_solid(255, 255, 255);
    const auto white_tex =
        device.create_texture({white.width, white.height, vyro::TextureFormat::RGBA8,
                               white.pixels.data()});

    const vyro::f32 g = 30.0f;
    const std::vector<vyro::Vertex3D> ground_verts = {
        {{-g, -0.9f, -g}, {0, 1, 0}, {0, 0}, {1, 1, 1}},
        {{g, -0.9f, -g}, {0, 1, 0}, {16, 0}, {1, 1, 1}},
        {{g, -0.9f, g}, {0, 1, 0}, {16, 16}, {1, 1, 1}},
        {{-g, -0.9f, g}, {0, 1, 0}, {0, 16}, {1, 1, 1}},
    };
    const std::vector<vyro::u32> ground_idx = {0, 1, 2, 0, 2, 3};
    vyro::MeshData ground_mesh;
    ground_mesh.vertices = ground_verts;
    ground_mesh.indices = ground_idx;
    const GpuMesh ground = upload(device, ground_mesh);

    // ── Camera: behind and above the player ──────────────────────────
    const vyro::f32 aspect = static_cast<vyro::f32>(window.framebuffer_width())
                             / static_cast<vyro::f32>(window.framebuffer_height());
    vyro::Camera camera;
    camera.set_perspective(1.0472f, aspect, 0.1f, 200.0f);
    camera.set_view({0.0f, 4.5f, kPlayerZ + 6.5f}, {0.0f, 0.0f, kPlayerZ - 8.0f});

    // ── World ────────────────────────────────────────────────────────
    vyro::Registry world;
    GameState state;
    std::mt19937 rng{1234};
    std::uniform_real_distribution<vyro::f32> spawn_x(-kArenaHalfWidth, kArenaHalfWidth);

    auto reset = [&] {
        std::vector<vyro::Entity> doomed;
        world.view<Position>().for_each_entity(
            [&](vyro::Entity e, Position&) { doomed.push_back(e); });
        for (const auto e : doomed) {
            world.destroy_entity(e);
        }
        state = GameState{};
        VYRO_INFO("Game", "new game");
    };

    auto draw_mesh = [&](const GpuMesh& m, const vyro::Mat4& model, vyro::TextureHandle tex) {
        device.set_uniform_mat4(shader, "u_mvp", camera.view_projection() * model);
        device.set_uniform_mat4(shader, "u_model", model);
        vyro::DrawCommand cmd;
        cmd.shader = shader;
        cmd.vertex_buffer = m.vbo;
        cmd.index_buffer = m.ibo;
        cmd.texture = tex;
        cmd.index_count = m.index_count;
        cmd.vertex_format = vyro::VertexFormat::Pos3Normal3UV2;
        device.draw(cmd);
    };

    auto last = std::chrono::steady_clock::now();
    int last_title_score = -1;

    while (window.is_open()) {
        const auto now = std::chrono::steady_clock::now();
        const vyro::f32 dt = std::min(std::chrono::duration<vyro::f32>(now - last).count(), 0.05f);
        last = now;

        window.poll_events();
        pump_input(input, window.native());

        if (input.is_action_pressed("Quit")) {
            window.close();
        }
        if (input.is_action_pressed("Restart")) {
            reset();
        }

        if (!state.game_over) {
            // ── Player movement ──────────────────────────────────────
            vyro::f32 move = 0.0f;
            if (input.is_action_down("MoveLeft")) {
                move -= 1.0f;
            }
            if (input.is_action_down("MoveRight")) {
                move += 1.0f;
            }
            state.player_x = std::clamp(state.player_x + move * kPlayerSpeed * dt,
                                        -kArenaHalfWidth, kArenaHalfWidth);

            // ── Shooting ─────────────────────────────────────────────
            state.fire_timer -= dt;
            if (input.is_action_down("Fire") && state.fire_timer <= 0.0f) {
                state.fire_timer = kFireCooldown;
                const auto b = world.create_entity();
                world.add_component<Position>(b, Position{{state.player_x, 0.0f, kPlayerZ - 0.8f}});
                world.add_component<Velocity>(b, Velocity{{0.0f, 0.0f, -kBulletSpeed}});
                world.add_component<BulletTag>(b, BulletTag{});
            }

            // ── Enemy spawning (difficulty ramps with score) ─────────
            state.spawn_timer -= dt;
            if (state.spawn_timer <= 0.0f) {
                state.spawn_timer = state.spawn_interval;
                state.spawn_interval = std::max(0.55f, 1.4f - static_cast<vyro::f32>(state.score) * 0.02f);
                state.enemy_speed = 5.0f + static_cast<vyro::f32>(state.score) * 0.08f;
                const auto e = world.create_entity();
                world.add_component<Position>(e, Position{{spawn_x(rng), 0.0f, kSpawnZ}});
                world.add_component<Velocity>(e, Velocity{{0.0f, 0.0f, state.enemy_speed}});
                world.add_component<EnemyTag>(e, EnemyTag{});
            }

            // ── Movement system ──────────────────────────────────────
            world.view<Position, Velocity>().for_each(
                [dt](Position& p, Velocity& v) { p.value = p.value + v.value * dt; });

            // ── Collision + cleanup systems (engine sphere tests) ────
            std::vector<vyro::Entity> dead;
            world.view<Position, EnemyTag>().for_each_entity([&](vyro::Entity e, Position& ep,
                                                                 EnemyTag&) {
                const vyro::Sphere enemy_sphere{ep.value, kEnemyRadius};

                // Enemy reached / rammed the player?
                const vyro::Sphere player_sphere{{state.player_x, 0.0f, kPlayerZ}, kPlayerRadius};
                if (vyro::collide(enemy_sphere, player_sphere).colliding
                    || ep.value.z > kPlayerZ + 1.5f) {
                    state.game_over = true;
                }

                // Bullet hits?
                world.view<Position, BulletTag>().for_each_entity(
                    [&](vyro::Entity b, Position& bp, BulletTag&) {
                        if (vyro::collide(enemy_sphere, vyro::Sphere{bp.value, kBulletRadius})
                                .colliding) {
                            dead.push_back(e);
                            dead.push_back(b);
                            ++state.score;
                        }
                    });
            });
            world.view<Position, BulletTag>().for_each_entity(
                [&](vyro::Entity b, Position& bp, BulletTag&) {
                    if (bp.value.z < kSpawnZ - 2.0f) {
                        dead.push_back(b);
                    }
                });
            for (const auto e : dead) {
                world.destroy_entity(e);
            }
            if (state.game_over) {
                VYRO_WARN("Game", "GAME OVER — score {} (press R to restart)", state.score);
            }
        }

        // ── Render ───────────────────────────────────────────────────
        device.set_viewport(window.framebuffer_width(), window.framebuffer_height());
        device.clear(state.game_over ? vyro::Vec4{0.25f, 0.05f, 0.06f, 1.0f}
                                     : vyro::Vec4{0.05f, 0.06f, 0.10f, 1.0f});
        device.set_uniform_vec3(shader, "u_lightDir", {-0.4f, -1.0f, -0.3f});

        draw_mesh(ground, vyro::Mat4::identity(), checker_tex);

        // Player ship: nose toward the enemies (-z).
        const vyro::Mat4 ship_model = vyro::Mat4::translation({state.player_x, 0.0f, kPlayerZ})
                                      * vyro::Mat4::rotation({0, 1, 0}, 3.14159265f) * ship_fit;
        draw_mesh(ship, ship_model, white_tex);

        world.view<Position, EnemyTag>().for_each([&](Position& p, EnemyTag&) {
            draw_mesh(enemy, vyro::Mat4::translation(p.value), white_tex);
        });
        world.view<Position, BulletTag>().for_each([&](Position& p, BulletTag&) {
            draw_mesh(bullet, vyro::Mat4::translation(p.value), white_tex);
        });

        // Score in the title bar (the engine has no text rendering yet).
        if (state.score != last_title_score) {
            last_title_score = state.score;
            glfwSetWindowTitle(window.native(),
                               std::format("VyroStrike — score {}", state.score).c_str());
        }

        window.swap_buffers();
    }

    VYRO_INFO("Game", "final score {}", state.score);
    return 0;
}
