// VyroStrike: Outbreak — a zombie shooter built on VyroEngine.
// You are the soldier. The horde shambles toward you — hold the line.
// Built entirely from engine systems:
//   ECS Registry/View   — bullets and zombies are entities
//   Input               — rebindable action map fed from the window's keys
//   Physics collision   — sphere-sphere hit tests
//   GLB/glTF assets     — textured soldier and zombie models (Poly Pizza)
//   OpenGL RHI          — textured, lit 3D rendering
//
// Controls:  A/D or Left/Right = move   Space = shoot   R = restart   Esc = quit
#include "vyro/assets/Image.hpp"
#include "vyro/assets/Mesh.hpp"
#include "vyro/assets/GlbLoader.hpp"
#include "vyro/assets/ObjLoader.hpp"
#include "vyro/assets/SkinnedModel.hpp"
#include "vyro/audio/AudioDevice.hpp"
#include "vyro/audio/AudioFile.hpp"
#include "vyro/audio/SoundSynth.hpp"
#include "vyro/core/Engine.hpp"
#include "vyro/core/Log.hpp"
#include "vyro/ecs/Registry.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/net/Coop.hpp"
#include "vyro/net/UdpTransport.hpp"
#include "vyro/physics/Collision.hpp"
#include "vyro/platform/Input.hpp"
#include "vyro/platform/Window.hpp"
#include "vyro/render/Camera.hpp"
#include "vyro/render/OpenGLDevice.hpp"
#include "vyro/render/ParticleSystem.hpp"
#include "vyro/render/TextGeometry.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <format>
#include <memory>
#include <random>
#include <string>
#include <string_view>
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
// A zombie that has been shot: plays out its death (fall + sink) then despawns.
struct DyingTag {
    vyro::f32 t = 0.0f;
};

// ── Tuning ───────────────────────────────────────────────────────────
constexpr vyro::f32 kArenaHalfWidth = 5.0f;
constexpr vyro::f32 kPlayerZ = 4.0f;
constexpr vyro::f32 kPlayerSpeed = 6.5f;
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
    vyro::f32 enemy_speed = 2.2f;
    int score = 0;
    int hp = 3;
    int wave = 1;
    bool game_over = false;
};

constexpr vyro::f32 kDeathDuration = 1.1f;

// Feed the window's physical keys into the engine's action-mapped Input.
// VYRO_AUTOFIRE=1 holds the fire action down — a headless smoke-test hook.
void pump_input(vyro::Input& input, GLFWwindow* window)
{
    static const bool autofire = std::getenv("VYRO_AUTOFIRE") != nullptr;
    input.new_frame();
    auto feed = [&](int glfw_key, vyro::KeyCode code) {
        const bool forced = autofire && code == vyro::KeyCode::Space;
        input.on_key(code, forced || glfwGetKey(window, glfw_key) == GLFW_PRESS);
    };
    feed(GLFW_KEY_A, vyro::KeyCode::A);
    feed(GLFW_KEY_D, vyro::KeyCode::D);
    feed(GLFW_KEY_LEFT, vyro::KeyCode::Left);
    feed(GLFW_KEY_RIGHT, vyro::KeyCode::Right);
    feed(GLFW_KEY_SPACE, vyro::KeyCode::Space);
    feed(GLFW_KEY_R, vyro::KeyCode::R);
    feed(GLFW_KEY_ESCAPE, vyro::KeyCode::Escape);
}

// Load a GLB character/prop with its embedded texture (white fallback).
vyro::GlbModel load_character(const char* rel)
{
    for (const char* prefix : {"", "../"}) {
        if (auto r = vyro::load_glb(std::string(prefix) + rel); r.has_value()) {
            VYRO_INFO("Game", "loaded '{}' ({} verts, textured={})", rel,
                      r->mesh.vertices.size(), r->has_texture);
            return std::move(r.value());
        }
    }
    VYRO_WARN("Game", "missing '{}'; using cube", rel);
    vyro::GlbModel fallback;
    fallback.mesh = vyro::make_cube(1.0f);
    return fallback;
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
    VYRO_INFO("Game", "VyroStrike: Outbreak — A/D move, Space shoot, R restart, Esc quit");

    vyro::Window window(1100, 760, "VyroStrike: Outbreak — score 0");
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

    // ── Real audio: synthesized SFX through the output device ────────
    vyro::AudioDevice audio;
    const bool sound_on = audio.init();
    audio.set_master_gain(0.6f);
    const auto sfx_shot = vyro::synth::gunshot();
    const auto sfx_groan = vyro::synth::groan();
    const auto sfx_hit = vyro::synth::hit();

    // Background music: load a file if present, else a synthesized loop (V4.2).
    if (sound_on) {
        std::vector<vyro::f32> music;
        for (const char* mp : {"assets/audio/music.wav", "../assets/audio/music.wav",
                               "assets/audio/music.mp3", "../assets/audio/music.mp3"}) {
            if (auto loaded = vyro::load_audio_file(mp); loaded.has_value()) {
                VYRO_INFO("Game", "music from '{}'", mp);
                music = std::move(loaded.value());
                break;
            }
        }
        if (music.empty()) {
            music = vyro::synth::music_loop(96.0f, 4);
        }
        audio.play_looping(music, 0.35f);
    }

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

    // ── HUD: unlit screen-space text pipeline ────────────────────────
    const char* hud_vs = R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;  layout(location=3) in vec3 aColor;
out vec3 vColor;
void main(){ gl_Position=vec4(aPos,1.0); vColor=aColor; })";
    const char* hud_fs = R"(#version 330 core
in vec3 vColor; out vec4 FragColor;
void main(){ FragColor=vec4(vColor,1.0); })";
    const auto hud_shader = device.create_shader({hud_vs, hud_fs});

    // ── Particles (V4.1): world-space unlit colored quads ────────────
    const char* part_vs = R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;  layout(location=3) in vec3 aColor;
uniform mat4 u_vp;
out vec3 vColor;
void main(){ gl_Position=u_vp*vec4(aPos,1.0); vColor=aColor; })";
    const char* part_fs = R"(#version 330 core
in vec3 vColor; out vec4 FragColor;
void main(){ FragColor=vec4(vColor,1.0); })";
    const auto part_shader = device.create_shader({part_vs, part_fs});
    constexpr vyro::usize kPartMaxVerts = 60000;
    const auto part_vbo = device.create_buffer(
        {vyro::BufferType::Vertex, kPartMaxVerts * sizeof(vyro::Vertex3D), nullptr});
    vyro::ParticleSystem particles(2048);
    particles.set_gravity({0.0f, -5.0f, 0.0f});
    std::vector<vyro::Vertex3D> part_verts;
    constexpr vyro::usize kHudMaxVerts = 40000;
    const auto hud_vbo = device.create_buffer(
        {vyro::BufferType::Vertex, kHudMaxVerts * sizeof(vyro::Vertex3D), nullptr});
    std::vector<vyro::Vertex3D> hud_verts;
    const vyro::f32 hud_aspect = static_cast<vyro::f32>(window.framebuffer_width())
                                 / static_cast<vyro::f32>(window.framebuffer_height());

    // ── Assets: real people and guns (GLB with embedded textures) ────
    vyro::GlbModel soldier_model = load_character("assets/models/characters/soldier_shooting.glb");

    // The zombie loads RIGGED: clips sampled per frame, vertices CPU-skinned.
    vyro::SkinnedModel zombie_model;
    for (const char* prefix : {"", "../"}) {
        if (auto r = vyro::load_glb_skinned(std::string(prefix)
                                            + "assets/models/characters/zombie_animated.glb");
            r.has_value()) {
            zombie_model = std::move(r.value());
            break;
        }
    }
    for (const auto& clip : zombie_model.clips) {
        VYRO_INFO("Game", "zombie clip: '{}' ({}s)", clip.name, clip.duration);
    }
    int walk_clip = 0;
    for (const char* want : {"Walk", "Run"}) {
        bool found = false;
        for (vyro::usize c = 0; c < zombie_model.clips.size(); ++c) {
            if (zombie_model.clips[c].name.find(want) != std::string::npos) {
                walk_clip = static_cast<int>(c);
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    // Bite clip: blended in as the horde closes on the soldier (V4.3).
    int bite_clip = walk_clip;
    for (vyro::usize c = 0; c < zombie_model.clips.size(); ++c) {
        if (zombie_model.clips[c].name.find("Bite") != std::string::npos) {
            bite_clip = static_cast<int>(c);
            break;
        }
    }
    if (!zombie_model.clips.empty()) {
        VYRO_INFO("Game", "horde animation: walk '{}' -> bite '{}'",
                  zombie_model.clips[static_cast<vyro::usize>(walk_clip)].name,
                  zombie_model.clips[static_cast<vyro::usize>(bite_clip)].name);
    }

    const GpuMesh soldier = upload(device, soldier_model.mesh);
    const vyro::Mat4 soldier_fit = fit_transform(soldier_model.mesh, 1.8f);
    const GpuMesh zombie = upload(device, zombie_model.mesh);
    std::vector<vyro::Mat4> zombie_pose;
    std::vector<vyro::Vertex3D> zombie_skinned;

    // Fit using a SKINNED frame: skinning bakes the rig's scale, so the raw
    // bind-pose extents are the wrong basis for normalization.
    vyro::Mat4 zombie_fit = vyro::Mat4::identity();
    if (!zombie_model.clips.empty()) {
        zombie_model.pose(0, 0.0f, zombie_pose);
        zombie_model.skin(zombie_pose, zombie_skinned);
        vyro::MeshData skinned_frame;
        skinned_frame.vertices = zombie_skinned;
        zombie_fit = fit_transform(skinned_frame, 1.9f);
    } else {
        zombie_fit = fit_transform(zombie_model.mesh, 1.9f);
    }

    auto upload_tex = [&](const vyro::Image& img, bool has) {
        if (has) {
            return device.create_texture(
                {img.width, img.height, vyro::TextureFormat::RGBA8, img.pixels.data()});
        }
        const vyro::Image white_px = vyro::make_solid(255, 255, 255);
        return device.create_texture(
            {white_px.width, white_px.height, vyro::TextureFormat::RGBA8, white_px.pixels.data()});
    };
    const auto soldier_tex = upload_tex(soldier_model.texture, soldier_model.has_texture);
    const auto zombie_tex = upload_tex(zombie_model.texture, zombie_model.has_texture);

    const GpuMesh bullet = upload(device, tinted_cube(0.22f, {1.0f, 0.85f, 0.25f}));

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

    // Co-op spawns the two allies apart so they don't overlap (set below).
    vyro::f32 coop_start_x = 0.0f;
    auto reset = [&] {
        std::vector<vyro::Entity> doomed;
        world.view<Position>().for_each_entity(
            [&](vyro::Entity e, Position&) { doomed.push_back(e); });
        for (const auto e : doomed) {
            world.destroy_entity(e);
        }
        state = GameState{};
        state.player_x = coop_start_x;
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

    // ── Co-op (V4.4) ─────────────────────────────────────────────────
    // VYRO_COOP=host|join brings a second networked soldier into the arena
    // over UDP. Both peers run the same CoopLink: publish my soldier, replicate
    // the other's. Default (unset) is unchanged single-player.
    //   host: binds 5555, talks to 127.0.0.1:5556, avatar id 1 (peer 2)
    //   join: binds 5556, talks to 127.0.0.1:5555, avatar id 2 (peer 1)
    // Override ports with VYRO_COOP_PORT / VYRO_COOP_PEER_PORT and the peer
    // host with VYRO_COOP_PEER_HOST.
    vyro::UdpTransport coop_socket;
    std::unique_ptr<vyro::CoopLink> coop;
    if (const char* mode = std::getenv("VYRO_COOP")) {
        const bool is_host = std::string_view(mode) == "host";
        const auto env_port = [](const char* name, vyro::u16 fallback) -> vyro::u16 {
            const char* v = std::getenv(name);
            return v != nullptr ? static_cast<vyro::u16>(std::atoi(v)) : fallback;
        };
        const vyro::u16 local_port = env_port("VYRO_COOP_PORT", is_host ? 5555 : 5556);
        const vyro::u16 peer_port = env_port("VYRO_COOP_PEER_PORT", is_host ? 5556 : 5555);
        const char* peer_host_env = std::getenv("VYRO_COOP_PEER_HOST");
        const std::string peer_host = peer_host_env != nullptr ? peer_host_env : "127.0.0.1";

        if (coop_socket.bind(local_port) && coop_socket.set_peer(peer_host, peer_port)) {
            coop = std::make_unique<vyro::CoopLink>(coop_socket, is_host ? 1u : 2u,
                                                    is_host ? 2u : 1u);
            coop_start_x = is_host ? -2.5f : 2.5f;
            state.player_x = coop_start_x;
            VYRO_INFO("Game", "co-op {}: bound {} -> {}:{}", mode, local_port, peer_host,
                      peer_port);
        } else {
            VYRO_WARN("Game", "co-op socket setup failed; staying single-player");
        }
    }

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
                if (sound_on) {
                    audio.play(sfx_shot, 0.8f);
                }
                const auto b = world.create_entity();
                world.add_component<Position>(b, Position{{state.player_x, 0.0f, kPlayerZ - 0.8f}});
                world.add_component<Velocity>(b, Velocity{{0.0f, 0.0f, -kBulletSpeed}});
                world.add_component<BulletTag>(b, BulletTag{});
                vyro::BurstParams muzzle;
                muzzle.origin = {state.player_x, 0.9f, kPlayerZ - 0.8f};
                muzzle.base_velocity = {0.0f, 0.2f, -6.0f};
                muzzle.speed = 2.2f;
                muzzle.lifetime = 0.18f;
                muzzle.size = 0.14f;
                muzzle.color = {1.0f, 0.85f, 0.35f};
                muzzle.count = 14;
                particles.burst(muzzle);
            }

            // ── Enemy spawning (difficulty ramps with score) ─────────
            state.wave = 1 + state.score / 10;
            state.spawn_timer -= dt;
            if (state.spawn_timer <= 0.0f) {
                state.spawn_timer = state.spawn_interval;
                state.spawn_interval =
                    std::max(0.5f, 1.5f - static_cast<vyro::f32>(state.wave) * 0.15f);
                state.enemy_speed = 2.0f + static_cast<vyro::f32>(state.wave) * 0.35f;
                const auto e = world.create_entity();
                world.add_component<Position>(e, Position{{spawn_x(rng), 0.0f, kSpawnZ}});
                world.add_component<Velocity>(e, Velocity{{0.0f, 0.0f, state.enemy_speed}});
                world.add_component<EnemyTag>(e, EnemyTag{});
            }

            // Zombies home toward the soldier (and will face their velocity).
            world.view<Position, Velocity, EnemyTag>().for_each(
                [&](Position& p, Velocity& v, EnemyTag&) {
                    const vyro::f32 dx = state.player_x - p.value.x;
                    v.value.x = std::clamp(dx * 0.5f, -1.6f, 1.6f);
                });

            // ── Movement system ──────────────────────────────────────
            world.view<Position, Velocity>().for_each(
                [dt](Position& p, Velocity& v) { p.value = p.value + v.value * dt; });

            // ── Collision + cleanup systems (engine sphere tests) ────
            std::vector<vyro::Entity> dead;
            std::vector<vyro::Entity> shot;
            world.view<Position, EnemyTag>().for_each_entity([&](vyro::Entity e, Position& ep,
                                                                 EnemyTag&) {
                if (world.has_component<DyingTag>(e)) {
                    return; // already falling — no bites, no double kills
                }
                const vyro::Sphere enemy_sphere{ep.value, kEnemyRadius};

                // A zombie that reaches the soldier takes a bite: lose a heart.
                const vyro::Sphere player_sphere{{state.player_x, 0.0f, kPlayerZ}, kPlayerRadius};
                if (vyro::collide(enemy_sphere, player_sphere).colliding
                    || ep.value.z > kPlayerZ + 1.5f) {
                    dead.push_back(e);
                    --state.hp;
                    if (sound_on) {
                        audio.play(sfx_hit, 1.0f);
                    }
                    if (state.hp <= 0) {
                        state.game_over = true;
                    }
                    return;
                }

                // Bullet hits start the death animation.
                world.view<Position, BulletTag>().for_each_entity(
                    [&](vyro::Entity b, Position& bp, BulletTag&) {
                        if (vyro::collide(enemy_sphere, vyro::Sphere{bp.value, kBulletRadius})
                                .colliding) {
                            shot.push_back(e);
                            dead.push_back(b);
                            ++state.score;
                        }
                    });
            });
            for (const auto e : shot) {
                if (!world.has_component<DyingTag>(e)) {
                    if (sound_on) {
                        audio.play(sfx_groan, 0.9f);
                    }
                    {
                        const auto* zp = world.get_component<Position>(e);
                        vyro::BurstParams blood;
                        blood.origin = zp != nullptr ? vyro::Vec3{zp->value.x, 0.9f, zp->value.z}
                                                     : vyro::Vec3{};
                        blood.speed = 4.0f;
                        blood.lifetime = 0.6f;
                        blood.size = 0.16f;
                        blood.color = {0.7f, 0.05f, 0.07f};
                        blood.count = 28;
                        particles.burst(blood);
                    }
                    world.add_component<DyingTag>(e, DyingTag{});
                    if (auto* v = world.get_component<Velocity>(e)) {
                        v->value = {};
                    }
                }
            }

            // Death playback: advance, then despawn when the fall completes.
            world.view<DyingTag>().for_each([dt](DyingTag& d) { d.t += dt; });
            world.view<Position, DyingTag>().for_each_entity(
                [&](vyro::Entity e, Position&, DyingTag& d) {
                    if (d.t >= kDeathDuration) {
                        dead.push_back(e);
                    }
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

        // Particles age even after game over so the last bursts finish.
        particles.update(dt);

        // ── Render ───────────────────────────────────────────────────
        device.set_viewport(window.framebuffer_width(), window.framebuffer_height());
        device.clear(state.game_over ? vyro::Vec4{0.25f, 0.05f, 0.06f, 1.0f}
                                     : vyro::Vec4{0.05f, 0.06f, 0.10f, 1.0f});
        device.set_uniform_vec3(shader, "u_lightDir", {-0.4f, -1.0f, -0.3f});

        draw_mesh(ground, vyro::Mat4::identity(), checker_tex);

        // Co-op tick: publish our soldier and replicate the peer's.
        if (coop) {
            coop->set_local_position({state.player_x, 0.0f, kPlayerZ});
            coop->broadcast();
            coop->poll();
        }

        // The soldier: feet on the ground, rifle facing the horde (-z).
        const vyro::Mat4 soldier_pose = vyro::Mat4::translation({state.player_x, 0.0f, kPlayerZ})
                                        * vyro::Mat4::rotation({0, 1, 0}, 3.14159265f)
                                        * soldier_fit;
        draw_mesh(soldier, soldier_pose, soldier_tex);

        // The co-op ally: a second soldier at the peer's replicated position.
        if (coop && coop->peer_connected()) {
            const vyro::Vec3 ally = coop->peer_position();
            const vyro::Mat4 ally_pose = vyro::Mat4::translation(ally)
                                         * vyro::Mat4::rotation({0, 1, 0}, 3.14159265f)
                                         * soldier_fit;
            draw_mesh(soldier, ally_pose, soldier_tex);
        }

        // How close is the nearest living zombie to the soldier? The horde
        // shares one skinned stream, so this drives a single walk->bite blend.
        vyro::f32 nearest = 1e9f;
        world.view<Position, EnemyTag>().for_each_entity(
            [&](vyro::Entity e, Position& p, EnemyTag&) {
                if (world.has_component<DyingTag>(e)) {
                    return;
                }
                const vyro::f32 dx = p.value.x - state.player_x;
                const vyro::f32 dz = p.value.z - kPlayerZ;
                nearest = std::min(nearest, std::sqrt(dx * dx + dz * dz));
            });

        // Animate the horde: cross-fade walk->bite by proximity, skin, stream.
        if (!zombie_model.clips.empty()) {
            const float anim_t =
                std::chrono::duration<float>(now.time_since_epoch()).count();
            // Full walk beyond kBiteFar, full bite within kBiteNear.
            constexpr vyro::f32 kBiteNear = 1.6f;
            constexpr vyro::f32 kBiteFar = 3.2f;
            const vyro::f32 bite_weight =
                std::clamp((kBiteFar - nearest) / (kBiteFar - kBiteNear), 0.0f, 1.0f);
            zombie_model.pose_blend(static_cast<vyro::usize>(walk_clip), anim_t,
                                    static_cast<vyro::usize>(bite_clip), anim_t, bite_weight,
                                    zombie_pose);
            zombie_model.skin(zombie_pose, zombie_skinned);
            device.update_buffer(zombie.vbo, zombie_skinned.data(),
                                 zombie_skinned.size() * sizeof(vyro::Vertex3D));
        }
        world.view<Position, EnemyTag>().for_each_entity([&](vyro::Entity e, Position& p,
                                                             EnemyTag&) {
            // Face the direction of travel (toward the soldier).
            vyro::f32 yaw = 0.0f;
            if (const auto* v = world.get_component<Velocity>(e)) {
                yaw = std::atan2(v->value.x, v->value.z);
            }
            vyro::Mat4 pose = vyro::Mat4::translation(p.value)
                              * vyro::Mat4::rotation({0, 1, 0}, yaw) * zombie_fit;

            // Death: tip backward and sink into the ground over the fall.
            if (const auto* dying = world.get_component<DyingTag>(e)) {
                const vyro::f32 k = std::min(dying->t / kDeathDuration, 1.0f);
                pose = vyro::Mat4::translation({p.value.x, -k * 1.2f, p.value.z})
                       * vyro::Mat4::rotation({0, 1, 0}, yaw)
                       * vyro::Mat4::rotation({1, 0, 0}, -k * 1.45f) * zombie_fit;
            }
            draw_mesh(zombie, pose, zombie_tex);
        });
        world.view<Position, BulletTag>().for_each([&](Position& p, BulletTag&) {
            draw_mesh(bullet,
                      vyro::Mat4::translation({p.value.x, 0.9f, p.value.z}), white_tex);
        });

        // Particles: billboard quads facing the camera (V4.1).
        if (particles.alive_count() > 0) {
            const vyro::Vec3 eye{0.0f, 4.5f, kPlayerZ + 6.5f};
            const vyro::Vec3 target{0.0f, 0.0f, kPlayerZ - 8.0f};
            const vyro::Vec3 fwd = vyro::normalize(target - eye);
            const vyro::Vec3 cam_right = vyro::normalize(vyro::cross(fwd, {0, 1, 0}));
            const vyro::Vec3 cam_up = vyro::cross(cam_right, fwd);
            part_verts.clear();
            particles.build_quads(cam_right, cam_up, part_verts);
            if (part_verts.size() > kPartMaxVerts) {
                part_verts.resize(kPartMaxVerts);
            }
            device.update_buffer(part_vbo, part_verts.data(),
                                 part_verts.size() * sizeof(vyro::Vertex3D));
            device.set_uniform_mat4(part_shader, "u_vp", camera.view_projection());
            vyro::DrawCommand pcmd;
            pcmd.shader = part_shader;
            pcmd.vertex_buffer = part_vbo;
            pcmd.vertex_count = static_cast<vyro::u32>(part_verts.size());
            pcmd.vertex_format = vyro::VertexFormat::Pos3Normal3UV2;
            device.draw(pcmd);
        }

        // ── On-screen HUD (V3.4): score, wave, hearts, game over ─────
        hud_verts.clear();
        vyro::text::build(std::format("WAVE {}", state.wave), -0.97f, 0.95f, 0.07f,
                          hud_aspect, {0.85f, 0.85f, 0.9f}, hud_verts);
        vyro::text::build(std::format("SCORE {}", state.score), -0.97f, 0.85f, 0.07f,
                          hud_aspect, {1.0f, 0.85f, 0.3f}, hud_verts);
        std::string hearts;
        for (int i = 0; i < std::max(state.hp, 0); ++i) {
            hearts += '\x03';
        }
        vyro::text::build(hearts, 0.7f, 0.95f, 0.09f, hud_aspect, {0.95f, 0.2f, 0.25f},
                          hud_verts);
        if (state.game_over) {
            const char* msg = "GAME OVER";
            const char* sub = "PRESS R TO RESTART";
            const vyro::f32 w1 = vyro::text::measure(msg, 0.22f, hud_aspect);
            const vyro::f32 w2 = vyro::text::measure(sub, 0.07f, hud_aspect);
            vyro::text::build(msg, -w1 * 0.5f, 0.2f, 0.22f, hud_aspect,
                              {0.95f, 0.15f, 0.15f}, hud_verts);
            vyro::text::build(sub, -w2 * 0.5f, -0.05f, 0.07f, hud_aspect,
                              {0.9f, 0.9f, 0.9f}, hud_verts);
        }
        if (hud_verts.size() > kHudMaxVerts) {
            hud_verts.resize(kHudMaxVerts);
        }
        device.update_buffer(hud_vbo, hud_verts.data(),
                             hud_verts.size() * sizeof(vyro::Vertex3D));
        device.set_depth_test(false);
        vyro::DrawCommand hud_cmd;
        hud_cmd.shader = hud_shader;
        hud_cmd.vertex_buffer = hud_vbo;
        hud_cmd.vertex_count = static_cast<vyro::u32>(hud_verts.size());
        hud_cmd.vertex_format = vyro::VertexFormat::Pos3Normal3UV2;
        device.draw(hud_cmd);
        device.set_depth_test(true);
        (void)last_title_score;

        window.swap_buffers();
    }

    VYRO_INFO("Game", "final score {}", state.score);
    return 0;
}
