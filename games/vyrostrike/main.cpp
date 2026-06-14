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
#include "vyro/ai/Steering.hpp"
#include "vyro/animation/CycleVariation.hpp"
#include "vyro/assets/SkinnedModel.hpp"
#include "vyro/audio/AudioDevice.hpp"
#include "vyro/audio/AudioFile.hpp"
#include "vyro/audio/Spatial.hpp"
#include "vyro/audio/SoundSynth.hpp"
#include "vyro/core/Engine.hpp"
#include "vyro/core/FrameStats.hpp"
#include "vyro/core/Random.hpp"
#include "vyro/game/Difficulty.hpp"
#include "vyro/game/GameFlow.hpp"
#include "vyro/game/Pickup.hpp"
#include "vyro/game/RunStats.hpp"
#include "vyro/game/SaveData.hpp"
#include "vyro/game/Weapon.hpp"
#include "vyro/core/Log.hpp"
#include "vyro/core/Profiler.hpp"
#include "vyro/ecs/Registry.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/net/Coop.hpp"
#include "vyro/net/CoopState.hpp"
#include "vyro/net/UdpTransport.hpp"
#include "vyro/physics/Collision.hpp"
#include "vyro/platform/Input.hpp"
#include "vyro/platform/Window.hpp"
#include "vyro/render/Camera.hpp"
#include "vyro/render/CameraRig.hpp"
#include "vyro/render/Frustum.hpp"
#include "vyro/render/Instancing.hpp"
#include "vyro/render/OpenGLDevice.hpp"
#include "vyro/render/ParticleSystem.hpp"
#include "vyro/render/PostProcess.hpp"
#include "vyro/render/TextGeometry.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <fstream>
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
struct BulletDamage {
    int value = 1;
}; // V7.3: a bullet carries its weapon's damage
struct EnemyTag {};
struct PickupItem {
    vyro::game::PickupKind kind = vyro::game::PickupKind::Ammo;
}; // V8.2
// A zombie that has been shot: plays out its death (fall + sink) then despawns.
struct DyingTag {
    vyro::f32 t = 0.0f;
};

// ── Enemy archetypes (V7.2): walker / runner / brute ─────────────────
struct EnemyType {
    const char* name;
    vyro::f32 speed_mult; // multiplies the wave base speed
    int health;           // bullet hits to kill
    vyro::f32 scale;      // render + hit size
    int score;            // points on kill
    vyro::f32 weight;     // spawn likelihood
};
constexpr EnemyType kEnemyTypes[] = {
    {"walker", 1.0f, 1, 1.0f, 1, 60.0f},
    {"runner", 1.8f, 1, 0.8f, 2, 25.0f},
    {"brute", 0.6f, 4, 1.5f, 5, 15.0f},
};
// Per-enemy data: which archetype and remaining health.
struct Enemy {
    int type = 0;
    int health = 1;
    vyro::f32 speed = 2.0f;
};

// ── Tuning ───────────────────────────────────────────────────────────
constexpr vyro::f32 kArenaHalfWidth = 5.0f;
constexpr vyro::f32 kPlayerZ = 4.0f;
constexpr vyro::f32 kPlayerSpeed = 6.5f;
constexpr vyro::f32 kBulletSpeed = 18.0f;
constexpr vyro::f32 kSpawnZ = -28.0f;
constexpr vyro::f32 kEnemyRadius = 0.55f;
constexpr vyro::f32 kBulletRadius = 0.18f;
constexpr vyro::f32 kPlayerRadius = 0.6f;

struct GameState {
    vyro::f32 player_x = 0.0f;
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
    // Weapon selection (V7.3): 1/2/3 pick a loadout, Q reloads.
    input.bind_action("Weapon1", vyro::KeyCode::Num1);
    input.bind_action("Weapon2", vyro::KeyCode::Num2);
    input.bind_action("Weapon3", vyro::KeyCode::Num3);
    input.bind_action("Reload", vyro::KeyCode::Q);
    input.bind_action("CycleDifficulty", vyro::KeyCode::Tab); // V8.4

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

    // ── Shader: textured/lit + shadow-mapped (V5.2) ──────────────────
    const char* vs = R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;  layout(location=3) in vec3 aColor;
uniform mat4 u_mvp; uniform mat4 u_model; uniform mat4 u_lightVP;
out vec3 vNormal; out vec3 vColor; out vec2 vUV; out vec4 vLightPos;
void main(){ gl_Position=u_mvp*vec4(aPos,1.0); vNormal=mat3(u_model)*aNormal;
vColor=aColor; vUV=aUV; vLightPos=u_lightVP*u_model*vec4(aPos,1.0); })";
    // V6.2: samples a depth-texture shadow map with slope-scaled bias + 5x5 PCF.
    const char* fs = R"(#version 330 core
in vec3 vNormal; in vec3 vColor; in vec2 vUV; in vec4 vLightPos; out vec4 FragColor;
uniform vec3 u_lightDir; uniform sampler2D u_texture; uniform sampler2D u_shadowMap;
uniform int u_shadowOn;
float shadow_factor(float ndl){
  if(u_shadowOn==0) return 0.0;
  vec3 p = vLightPos.xyz / vLightPos.w * 0.5 + 0.5;
  if(p.z>1.0 || p.x<0.0 || p.x>1.0 || p.y<0.0 || p.y>1.0) return 0.0;
  // matches shadows::slope_scaled_bias(ndl, 0.0012, 0.004)
  float bias=clamp(0.0012+0.004*(1.0-ndl),0.0012,0.0052);
  float sm=0.0; vec2 ts=1.0/vec2(textureSize(u_shadowMap,0));
  for(int x=-2;x<=2;++x) for(int y=-2;y<=2;++y){
    float closest=texture(u_shadowMap,p.xy+vec2(x,y)*ts).r;
    sm += (p.z-bias>closest)?1.0:0.0;
  }
  return sm/25.0;
}
void main(){ vec3 N=normalize(vNormal); float d=max(dot(N,normalize(-u_lightDir)),0.0);
vec3 base=texture(u_texture,vUV).rgb*vColor;
float lit=(0.35+0.65*d)*(1.0-0.6*shadow_factor(d));
FragColor=vec4(base*lit,1.0); })";
    const auto shader = device.create_shader({vs, fs});

    // Depth-only shader for the shadow pass: write window-space depth to color.
    const char* shadow_vs = R"(#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 u_mvp;
void main(){ gl_Position=u_mvp*vec4(aPos,1.0); })";
    const char* shadow_fs = R"(#version 330 core
out vec4 FragColor;
void main(){ FragColor=vec4(gl_FragCoord.z,0.0,0.0,1.0); })";
    const auto shadow_shader = device.create_shader({shadow_vs, shadow_fs});
    // Depth-texture shadow map (V6.2): sampled directly, no color encoding.
    const auto shadow_rt =
        device.create_render_target({2048, 2048, /*hdr*/ false, /*depth_texture*/ true});

    // Instanced shader (V6.1): per-instance model matrix in attributes 4-7;
    // same lit + shadow fragment path as the main shader.
    const char* inst_vs = R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;  layout(location=3) in vec3 aColor;
layout(location=4) in vec4 aI0; layout(location=5) in vec4 aI1;
layout(location=6) in vec4 aI2; layout(location=7) in vec4 aI3;
uniform mat4 u_vp; uniform mat4 u_lightVP;
out vec3 vNormal; out vec3 vColor; out vec2 vUV; out vec4 vLightPos;
void main(){ mat4 model=mat4(aI0,aI1,aI2,aI3);
gl_Position=u_vp*model*vec4(aPos,1.0); vNormal=mat3(model)*aNormal;
vColor=aColor; vUV=aUV; vLightPos=u_lightVP*model*vec4(aPos,1.0); })";
    const auto inst_shader = device.create_shader({inst_vs, fs});

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

    // ── Post-FX (V5.1): HDR bloom composite ──────────────────────────
    // The scene renders into an offscreen HDR target; this fullscreen pass
    // samples it, blooms the bright (>1) pixels, ACES-tonemaps to LDR, then
    // applies the V4.5 vignette and damage flash. u_scene = scene color,
    // u_flash = red damage pulse.
    const char* post_vs = R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=2) in vec2 aUV;
out vec2 vUV;
void main(){ gl_Position=vec4(aPos,1.0); vUV=aUV; })";
    const char* post_fs = R"(#version 330 core
in vec2 vUV; out vec4 FragColor;
uniform sampler2D u_scene; uniform float u_flash;
void main(){
  ivec2 sz = textureSize(u_scene, 0);
  vec2 texel = 1.0 / vec2(sz);
  vec3 hdr = texture(u_scene, vUV).rgb;
  // Cheap separable-ish bloom: gather the bright component over a neighborhood.
  vec3 bloom = vec3(0.0); float total = 0.0;
  for (int dx=-3; dx<=3; ++dx){
    for (int dy=-3; dy<=3; ++dy){
      float w = 1.0 / (1.0 + float(dx*dx+dy*dy));
      vec3 s = texture(u_scene, vUV + vec2(dx,dy)*texel*2.5).rgb;
      bloom += max(s - vec3(1.0), vec3(0.0)) * w; // threshold 1.0
      total += w;
    }
  }
  bloom /= total;
  vec3 color = hdr + bloom * 1.4;
  // ACES filmic tonemap (Narkowicz), matching postfx::tonemap_aces.
  vec3 m = clamp((color*(2.51*color+0.03))/(color*(2.43*color+0.59)+0.14), 0.0, 1.0);
  // Vignette + damage flash.
  float d = distance(vUV, vec2(0.5));
  m *= (1.0 - smoothstep(0.45, 0.85, d) * 0.45);
  m = mix(m, vec3(0.75,0.05,0.06), u_flash);
  FragColor = vec4(m, 1.0);
})";
    const auto post_shader = device.create_shader({post_vs, post_fs});
    // HDR offscreen target the whole scene renders into (V5.1).
    const auto scene_rt = device.create_render_target(
        {window.framebuffer_width(), window.framebuffer_height(), /*hdr*/ true});
    // A fullscreen quad in NDC; uv 0..1 across the screen for the vignette.
    auto make_quad_vertex = [](vyro::f32 x, vyro::f32 y, vyro::f32 u, vyro::f32 v) {
        vyro::Vertex3D vert;
        vert.position = {x, y, 0.0f};
        vert.uv = {u, v};
        return vert;
    };
    const std::vector<vyro::Vertex3D> post_verts{
        make_quad_vertex(-1, -1, 0, 0), make_quad_vertex(1, -1, 1, 0),
        make_quad_vertex(1, 1, 1, 1),   make_quad_vertex(-1, -1, 0, 0),
        make_quad_vertex(1, 1, 1, 1),   make_quad_vertex(-1, 1, 0, 1)};
    const auto post_vbo = device.create_buffer(
        {vyro::BufferType::Vertex, post_verts.size() * sizeof(vyro::Vertex3D), post_verts.data()});

    // ── Particles (V4.1): world-space unlit colored quads ────────────
    const char* part_vs = R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;  layout(location=3) in vec3 aColor;
uniform mat4 u_vp;
out vec3 vColor;
void main(){ gl_Position=u_vp*vec4(aPos,1.0); vColor=aColor; })";
    // Particles are emissive (>1) so the HDR bloom pass makes them glow.
    const char* part_fs = R"(#version 330 core
in vec3 vColor; out vec4 FragColor;
void main(){ FragColor=vec4(vColor*3.0,1.0); })";
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

    // V6.1: GPU hardware instancing — the skinned zombie mesh is drawn once per
    // instance, each reading a per-instance model matrix from this buffer.
    constexpr vyro::usize kMaxZombieInstances = 64;
    const auto horde_instance_vbo = device.create_buffer(
        {vyro::BufferType::Vertex, vyro::instancing::buffer_bytes(kMaxZombieInstances), nullptr});

    // V7.1: spread the horde across K phase buckets so they don't march in
    // lockstep. Each bucket has its own skinned pose buffer (animated at an
    // offset of the cycle) and is drawn as a separate instanced batch.
    constexpr vyro::u32 kPhaseBuckets = 5;
    std::array<vyro::BufferHandle, kPhaseBuckets> pose_vbos;
    for (auto& vb : pose_vbos) {
        vb = device.create_buffer({vyro::BufferType::Vertex,
                                   zombie_model.mesh.vertices.size() * sizeof(vyro::Vertex3D),
                                   nullptr});
    }
    std::array<std::vector<vyro::Mat4>, kPhaseBuckets> bucket_xforms;

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

    // Pickup cubes (V8.2): green health, yellow ammo, cyan score-boost.
    const std::array<GpuMesh, 3> pickup_meshes{
        upload(device, tinted_cube(0.4f, {0.2f, 0.95f, 0.3f})),
        upload(device, tinted_cube(0.4f, {1.0f, 0.85f, 0.2f})),
        upload(device, tinted_cube(0.4f, {0.3f, 0.8f, 1.0f}))};

    // ── Level obstacles (V6.4): pillars that occlude, block, and divert ──
    // A simple authored layout (a future pass loads these via SceneSerializer).
    const GpuMesh pillar = upload(device, tinted_cube(1.0f, {0.45f, 0.42f, 0.5f}));
    const std::vector<vyro::ai::Obstacle> obstacles = {
        {{-4.0f, 0.0f, kPlayerZ - 9.0f}, 1.3f},
        {{4.5f, 0.0f, kPlayerZ - 12.0f}, 1.3f},
        {{-1.0f, 0.0f, kPlayerZ - 16.0f}, 1.3f},
    };
    // Pillar model: a tall box at the obstacle, base on the ground.
    const auto pillar_model = [](const vyro::ai::Obstacle& o) {
        return vyro::Mat4::translation({o.center.x, 0.6f, o.center.z})
               * vyro::Mat4::scale({o.radius * 1.4f, 3.0f, o.radius * 1.4f});
    };

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

    // V8.5: waves are data-driven — loaded from "waves.txt" if present, else a
    // built-in default. Each line is "kills,intermission".
    const char* kDefaultWaves = "8,3\n13,3\n18,2.5\n24,2.5\n32,2\n";
    std::string waves_text;
    {
        std::ifstream wf("waves.txt");
        if (wf.is_open()) {
            waves_text.assign((std::istreambuf_iterator<char>(wf)),
                              std::istreambuf_iterator<char>());
        }
    }
    auto wave_plans = vyro::game::parse_wave_plans(waves_text.empty() ? kDefaultWaves : waves_text);
    if (wave_plans.empty()) {
        wave_plans = vyro::game::parse_wave_plans(kDefaultWaves);
    }
    VYRO_INFO("Game", "loaded {} waves", wave_plans.size());
    vyro::game::GameFlow flow(wave_plans);
    vyro::game::RunStats stats; // V8.3: per-run performance

    // V8.1: persisted profile (best score/wave + settings) across runs.
    const std::string save_path = "vyrostrike.sav";
    vyro::game::SaveData save = vyro::game::load_from_file(save_path);
    bool result_saved = false; // save best score/wave once per game-over
    state.hp = vyro::game::difficulty_mods(save.difficulty).player_hp; // V8.4: starting health
    VYRO_INFO("Game", "loaded save: high score {}, best wave {}, difficulty {}", save.high_score,
              save.best_wave, save.difficulty);

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
        state.hp = vyro::game::difficulty_mods(save.difficulty).player_hp; // V8.4
        flow.reset();
        stats.reset();
        result_saved = false;
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

    // Shadow pass: draw a mesh's depth from the light's POV (V5.2).
    vyro::Mat4 light_vp = vyro::Mat4::identity();
    auto draw_shadow = [&](const GpuMesh& m, const vyro::Mat4& model) {
        device.set_uniform_mat4(shadow_shader, "u_mvp", light_vp * model);
        vyro::DrawCommand cmd;
        cmd.shader = shadow_shader;
        cmd.vertex_buffer = m.vbo;
        cmd.index_buffer = m.ibo;
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
    // V6.3: a second channel carries the host's authoritative world state
    // (score/wave/hp/horde) so the joiner sees the same fight.
    vyro::UdpTransport coop_state_socket;
    std::unique_ptr<vyro::CoopStateChannel> coop_state;
    bool coop_is_host = false;
    if (const char* mode = std::getenv("VYRO_COOP")) {
        const bool is_host = std::string_view(mode) == "host";
        coop_is_host = is_host;
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
            // World-state channel on the +100 ports.
            if (coop_state_socket.bind(static_cast<vyro::u16>(local_port + 100))
                && coop_state_socket.set_peer(peer_host,
                                              static_cast<vyro::u16>(peer_port + 100))) {
                coop_state = std::make_unique<vyro::CoopStateChannel>(coop_state_socket);
            }
        } else {
            VYRO_WARN("Game", "co-op socket setup failed; staying single-player");
        }
    }
    const bool coop_is_joiner = coop && !coop_is_host;

    // ── Camera rig (V4.5): smoothed follow + trauma-based screen shake ─
    vyro::ScreenShake camera_shake(1.6f);
    vyro::f32 cam_focus_x = state.player_x; // eases toward the soldier
    vyro::f32 damage_flash = 0.0f;          // red screen pulse on a bite
    const vyro::Vec3 base_eye{0.0f, 4.5f, kPlayerZ + 6.5f};
    const vyro::Vec3 base_target{0.0f, 0.0f, kPlayerZ - 8.0f};

    // ── Weapons (V7.3): pistol / rifle / shotgun loadouts ───────────────
    std::array<vyro::game::Weapon, 3> weapons{
        vyro::game::Weapon({"pistol", 0.22f, 1, 0.0f, 1, 12, 0.9f}),
        vyro::game::Weapon({"rifle", 0.09f, 1, 0.04f, 1, 30, 1.4f}),
        vyro::game::Weapon({"shotgun", 0.7f, 7, 0.5f, 2, 6, 1.6f})};
    int current_weapon = 0;

    // ── Profiling (V6.5): frame-time EMA + a short history for the graph ─
    vyro::FrameStats fps_stats(0.1);
    std::vector<vyro::f32> frame_history; // recent frame times (ms)
    constexpr vyro::usize kFrameHistory = 48;
    constexpr vyro::f64 kFrameBudgetMs = 16.6; // 60 fps

    auto last = std::chrono::steady_clock::now();
    int last_title_score = -1;

    while (window.is_open()) {
        const auto now = std::chrono::steady_clock::now();
        const vyro::f32 dt = std::min(std::chrono::duration<vyro::f32>(now - last).count(), 0.05f);
        last = now;

        // Frame timing for the on-screen profiler (V6.5).
        const vyro::f64 frame_ms = static_cast<vyro::f64>(dt) * 1000.0;
        fps_stats.add(frame_ms);
        vyro::Profiler::instance().end_frame(frame_ms);
        frame_history.push_back(static_cast<vyro::f32>(fps_stats.average_ms()));
        if (frame_history.size() > kFrameHistory) {
            frame_history.erase(frame_history.begin());
        }

        window.poll_events();
        pump_input(input, window.native());

        if (input.is_action_pressed("Quit")) {
            window.close();
        }
        if (input.is_action_pressed("Restart")) {
            reset();
        }
        // Cycle difficulty (V8.4); persisted, applies on the next run.
        if (input.is_action_pressed("CycleDifficulty")) {
            save.difficulty = vyro::game::next_difficulty(save.difficulty);
            vyro::game::save_to_file(save_path, save);
        }

        if (!state.game_over) {
            flow.update(dt);                                 // advance wave flow (V7.5)
            if (flow.phase() == vyro::game::Phase::Fighting) {
                stats.tick(dt); // time survived while fighting (V8.3)
            }
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
            // Obstacles block the soldier on the x-axis (V6.4).
            for (const auto& o : obstacles) {
                if (std::fabs(kPlayerZ - o.center.z) < o.radius + 0.4f) {
                    const vyro::f32 dx = state.player_x - o.center.x;
                    const vyro::f32 minx = o.radius + 0.4f;
                    if (std::fabs(dx) < minx) {
                        state.player_x = o.center.x + (dx >= 0.0f ? minx : -minx);
                    }
                }
            }

            // ── Weapons & shooting (V7.3) ────────────────────────────
            if (input.is_action_pressed("Weapon1")) {
                current_weapon = 0;
            }
            if (input.is_action_pressed("Weapon2")) {
                current_weapon = 1;
            }
            if (input.is_action_pressed("Weapon3")) {
                current_weapon = 2;
            }
            if (input.is_action_pressed("Reload")) {
                weapons[static_cast<vyro::usize>(current_weapon)].begin_reload();
            }
            vyro::game::Weapon& weapon = weapons[static_cast<vyro::usize>(current_weapon)];
            weapon.update(dt);
            if (input.is_action_down("Fire") && weapon.fire()) {
                const auto& ws = weapon.stats();
                if (sound_on) {
                    audio.play(sfx_shot, 0.8f);
                }
                stats.on_shots(ws.pellets); // V8.3: count projectiles fired
                // Spawn one bullet per pellet, fanned across the spread cone.
                for (const vyro::f32 ang : vyro::game::spread_angles(ws.pellets, ws.spread)) {
                    const auto b = world.create_entity();
                    world.add_component<Position>(
                        b, Position{{state.player_x, 0.0f, kPlayerZ - 0.8f}});
                    world.add_component<Velocity>(
                        b, Velocity{{std::sin(ang) * kBulletSpeed, 0.0f,
                                     -std::cos(ang) * kBulletSpeed}});
                    world.add_component<BulletTag>(b, BulletTag{});
                    world.add_component<BulletDamage>(b, BulletDamage{ws.damage});
                }
                vyro::BurstParams muzzle;
                muzzle.origin = {state.player_x, 0.9f, kPlayerZ - 0.8f};
                muzzle.base_velocity = {0.0f, 0.2f, -6.0f};
                muzzle.speed = 2.2f;
                muzzle.lifetime = 0.18f;
                muzzle.size = 0.14f;
                muzzle.color = {1.0f, 0.85f, 0.35f};
                muzzle.count = ws.pellets > 1 ? 22 : 14;
                particles.burst(muzzle);
                camera_shake.add_trauma(ws.pellets > 1 ? 0.35f : 0.16f);
            }
            // Auto-reload when the magazine runs dry.
            if (weapon.ammo() == 0 && !weapon.reloading()) {
                weapon.begin_reload();
            }

            // ── Enemy spawning (difficulty ramps with the wave) ──────
            // Only during the Fighting phase; the co-op joiner renders the
            // host's authoritative horde instead of spawning (V6.3/V7.5).
            state.wave = flow.wave();
            state.spawn_timer -= dt;
            if (flow.spawning() && !coop_is_joiner && state.spawn_timer <= 0.0f) {
                state.spawn_timer = state.spawn_interval;
                state.spawn_interval =
                    std::max(0.5f, 1.5f - static_cast<vyro::f32>(state.wave) * 0.15f);
                const auto diff = vyro::game::difficulty_mods(save.difficulty); // V8.4
                state.enemy_speed =
                    (2.0f + static_cast<vyro::f32>(state.wave) * 0.35f) * diff.enemy_speed;
                state.spawn_interval /= diff.spawn_rate; // faster on harder modes
                // Pick an archetype; runners/brutes get more common with waves.
                const vyro::f32 ramp = std::min(static_cast<vyro::f32>(state.wave) * 0.05f, 0.6f);
                const std::array<vyro::f32, 3> weights{
                    kEnemyTypes[0].weight * (1.0f - ramp), kEnemyTypes[1].weight * (1.0f + ramp),
                    kEnemyTypes[2].weight * (1.0f + ramp)};
                const auto roll = std::uniform_real_distribution<vyro::f32>(0.0f, 1.0f)(rng);
                const int type = static_cast<int>(vyro::weighted_index(weights, roll));
                const EnemyType& et = kEnemyTypes[type];
                const auto e = world.create_entity();
                world.add_component<Position>(e, Position{{spawn_x(rng), 0.0f, kSpawnZ}});
                world.add_component<Velocity>(e, Velocity{{0.0f, 0.0f, state.enemy_speed}});
                world.add_component<EnemyTag>(e, EnemyTag{});
                const int hp = std::max(
                    1, static_cast<int>(std::lround(static_cast<vyro::f32>(et.health)
                                                    * diff.enemy_health)));
                world.add_component<Enemy>(
                    e, Enemy{type, hp, state.enemy_speed * et.speed_mult});
            }

            // Zombie AI (V5.4): seek the soldier while spreading from the pack
            // (separation), so the horde fans out and surrounds instead of
            // collapsing into a single file.
            std::vector<vyro::Vec3> horde_positions;
            world.view<Position, EnemyTag>().for_each(
                [&](Position& p, EnemyTag&) { horde_positions.push_back(p.value); });
            const vyro::Vec3 soldier_pos{state.player_x, 0.0f, kPlayerZ};
            world.view<Position, Velocity, Enemy>().for_each(
                [&](Position& p, Velocity& v, Enemy& en) {
                    const vyro::f32 speed = en.speed; // per-archetype speed (V7.2)
                    const vyro::f32 dist = vyro::length(soldier_pos - p.value);
                    const vyro::ai::ZombieState st = vyro::ai::select_state(dist, 60.0f, 1.8f);
                    if (st == vyro::ai::ZombieState::Idle) {
                        v.value = {};
                        return;
                    }
                    vyro::Vec3 desired = vyro::ai::horde_velocity(
                        p.value, soldier_pos, horde_positions, speed, 2.5f, 1.5f);
                    desired = desired + vyro::ai::avoid_obstacles(p.value, obstacles, 2.0f, speed);
                    const vyro::f32 sp = vyro::length(desired);
                    if (sp > speed && sp > 1e-5f) {
                        desired = desired * (speed / sp);
                    }
                    v.value = desired;
                    v.value.y = 0.0f;
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
                auto* en = world.get_component<Enemy>(e);
                const vyro::f32 escale = en != nullptr ? kEnemyTypes[en->type].scale : 1.0f;
                const vyro::Sphere enemy_sphere{ep.value, kEnemyRadius * escale};

                // A zombie that reaches the soldier takes a bite: lose a heart.
                const vyro::Sphere player_sphere{{state.player_x, 0.0f, kPlayerZ}, kPlayerRadius};
                if (vyro::collide(enemy_sphere, player_sphere).colliding
                    || ep.value.z > kPlayerZ + 1.5f) {
                    dead.push_back(e);
                    --state.hp;
                    camera_shake.add_trauma(0.6f); // a hard hit kicks the camera
                    damage_flash = 0.7f;           // and flashes the screen red
                    if (sound_on) {
                        audio.play(sfx_hit, 1.0f);
                    }
                    if (state.hp <= 0) {
                        flow.player_died(); // -> Defeat (V7.5)
                    }
                    return;
                }

                // Bullet hits deal damage; the enemy only dies at 0 health (V7.2).
                world.view<Position, BulletTag>().for_each_entity(
                    [&](vyro::Entity b, Position& bp, BulletTag&) {
                        if (en == nullptr || en->health <= 0) {
                            return;
                        }
                        if (vyro::collide(enemy_sphere, vyro::Sphere{bp.value, kBulletRadius})
                                .colliding) {
                            dead.push_back(b); // bullet is consumed
                            stats.on_hit();    // V8.3: projectile connected
                            const auto* bd = world.get_component<BulletDamage>(b);
                            en->health -= bd != nullptr ? bd->value : 1;
                            if (en->health <= 0) {
                                shot.push_back(e);
                                state.score += kEnemyTypes[en->type].score;
                                stats.on_kill(en->type); // V8.3
                                flow.register_kill();    // wave progress (V7.5)
                            }
                        }
                    });
            });
            for (const auto e : shot) {
                if (!world.has_component<DyingTag>(e)) {
                    const auto* zpos = world.get_component<Position>(e);
                    if (sound_on) {
                        // Distance-attenuated death groan from the soldier (V7.4).
                        const vyro::Vec3 listener{state.player_x, 0.0f, kPlayerZ};
                        const vyro::f32 dist =
                            zpos != nullptr ? vyro::length(zpos->value - listener) : 0.0f;
                        const vyro::f32 gain = 0.9f * vyro::audio::attenuation(dist, 3.0f, 34.0f);
                        audio.play(sfx_groan, gain);
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
                    // Chance to drop a pickup at the kill site (V8.2).
                    if (!coop_is_joiner && zpos != nullptr) {
                        const auto roll = std::uniform_real_distribution<vyro::f32>(0, 1)(rng);
                        if (vyro::game::should_drop(roll, vyro::game::PickupTable{}.drop_chance)) {
                            const auto kroll = std::uniform_real_distribution<vyro::f32>(0, 1)(rng);
                            const auto kind = vyro::game::pick_kind(vyro::game::PickupTable{}, kroll);
                            const auto pk = world.create_entity();
                            world.add_component<Position>(
                                pk, Position{{zpos->value.x, 0.0f, zpos->value.z}});
                            world.add_component<PickupItem>(pk, PickupItem{kind});
                        }
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

            // ── Pickup collection (V8.2) ─────────────────────────────
            std::vector<vyro::Entity> collected;
            const vyro::Vec3 soldier_at{state.player_x, 0.0f, kPlayerZ};
            world.view<Position, PickupItem>().for_each_entity(
                [&](vyro::Entity pk, Position& pp, PickupItem& item) {
                    if (vyro::length(pp.value - soldier_at) > 1.1f) {
                        return;
                    }
                    switch (item.kind) {
                    case vyro::game::PickupKind::Health:
                        state.hp = std::min(state.hp + vyro::game::kHealthRestore, 3);
                        break;
                    case vyro::game::PickupKind::Ammo:
                        weapons[static_cast<vyro::usize>(current_weapon)].refill();
                        break;
                    case vyro::game::PickupKind::ScoreBoost:
                        state.score += vyro::game::kScoreBoost;
                        break;
                    }
                    if (sound_on) {
                        audio.play(sfx_shot, 0.5f);
                    }
                    collected.push_back(pk);
                });
            for (const auto pk : collected) {
                world.destroy_entity(pk);
            }

            // Victory or defeat freezes gameplay until restart (V7.5).
            if (flow.over()) {
                state.game_over = true;
                // Persist best score/wave once per run (V8.1).
                if (!result_saved) {
                    save.high_score = std::max(save.high_score, state.score);
                    save.best_wave = std::max(save.best_wave, flow.wave());
                    vyro::game::save_to_file(save_path, save);
                    result_saved = true;
                }
                VYRO_WARN("Game", "{} — score {} (best {}) (press R to restart)",
                          flow.phase() == vyro::game::Phase::Victory ? "VICTORY" : "DEFEAT",
                          state.score, save.high_score);
            }
        }

        // Particles age even after game over so the last bursts finish.
        particles.update(dt);

        // ── Camera rig (V4.5): ease toward the soldier, apply shake ──
        camera_shake.update(dt);
        damage_flash = std::max(0.0f, damage_flash - dt * 1.8f);
        cam_focus_x += (state.player_x - cam_focus_x) * vyro::smooth_factor(6.0f, dt);
        const vyro::Vec3 shake_off = camera_shake.offset(0.35f);
        const vyro::Vec3 cam_pan{cam_focus_x + shake_off.x, shake_off.y, 0.0f};
        camera.set_view(base_eye + cam_pan, base_target + cam_pan);

        device.reset_draw_count(); // count draw calls this frame (V5.5)

        // ── Shadow depth pass (V5.2): occluders from the sun's POV ───
        const vyro::Vec3 sun_dir{-0.4f, -1.0f, -0.3f};
        light_vp = vyro::shadows::light_view_projection(
            sun_dir, {state.player_x, 0.0f, kPlayerZ - 2.0f}, 20.0f, 40.0f);
        device.bind_render_target(shadow_rt);
        device.set_viewport(2048, 2048);
        device.clear({1.0f, 1.0f, 1.0f, 1.0f}); // far depth = 1
        {
            const vyro::Mat4 sp = vyro::Mat4::translation({state.player_x, 0.0f, kPlayerZ})
                                  * vyro::Mat4::rotation({0, 1, 0}, 3.14159265f) * soldier_fit;
            draw_shadow(soldier, sp);
            if (coop && coop->peer_connected()) {
                const vyro::Vec3 ally = coop->peer_position();
                draw_shadow(soldier, vyro::Mat4::translation(ally)
                                         * vyro::Mat4::rotation({0, 1, 0}, 3.14159265f)
                                         * soldier_fit);
            }
            world.view<Position, EnemyTag>().for_each_entity(
                [&](vyro::Entity e, Position& p, EnemyTag&) {
                    vyro::f32 yaw = 0.0f;
                    if (const auto* v = world.get_component<Velocity>(e)) {
                        yaw = std::atan2(v->value.x, v->value.z);
                    }
                    vyro::Mat4 pose = vyro::Mat4::translation(p.value)
                                      * vyro::Mat4::rotation({0, 1, 0}, yaw) * zombie_fit;
                    if (const auto* dying = world.get_component<DyingTag>(e)) {
                        const vyro::f32 k = std::min(dying->t / kDeathDuration, 1.0f);
                        pose = vyro::Mat4::translation({p.value.x, -k * 1.2f, p.value.z})
                               * vyro::Mat4::rotation({0, 1, 0}, yaw)
                               * vyro::Mat4::rotation({1, 0, 0}, -k * 1.45f) * zombie_fit;
                    }
                    draw_shadow(zombie, pose);
                });
            for (const auto& o : obstacles) {
                draw_shadow(pillar, pillar_model(o));
            }
        }

        // ── Render scene into the offscreen HDR target (V5.1) ────────
        device.bind_render_target(scene_rt);
        device.set_viewport(window.framebuffer_width(), window.framebuffer_height());
        device.clear(state.game_over ? vyro::Vec4{0.25f, 0.05f, 0.06f, 1.0f}
                                     : vyro::Vec4{0.05f, 0.06f, 0.10f, 1.0f});
        device.set_uniform_vec3(shader, "u_lightDir", sun_dir);
        // Bind the shadow map (unit 1) and light matrix for shadow lookups.
        device.set_uniform_mat4(shader, "u_lightVP", light_vp);
        device.bind_texture(device.render_target_texture(shadow_rt), 1);
        device.set_uniform_int(shader, "u_shadowMap", 1);
        device.set_uniform_int(shader, "u_shadowOn", 1);

        // ── Larger world + frustum culling (V5.3) ────────────────────
        // The arena is a grid of ground tiles; only the visible ones draw.
        const vyro::Frustum frustum = vyro::frustum_from_view_projection(camera.view_projection());
        int tiles_drawn = 0;
        int tiles_total = 0;
        constexpr vyro::f32 kTile = 60.0f; // ground mesh spans [-30,30]
        constexpr int kTileRadius = 3;     // 7x7 grid -> ~420-unit world
        for (int tz = -kTileRadius; tz <= kTileRadius; ++tz) {
            for (int tx = -kTileRadius; tx <= kTileRadius; ++tx) {
                ++tiles_total;
                const vyro::f32 cx = std::round(cam_focus_x / kTile) * kTile
                                     + static_cast<vyro::f32>(tx) * kTile;
                const vyro::f32 cz = kPlayerZ + static_cast<vyro::f32>(tz) * kTile;
                const vyro::Vec3 lo{cx - 30.0f, -0.95f, cz - 30.0f};
                const vyro::Vec3 hi{cx + 30.0f, -0.85f, cz + 30.0f};
                if (!vyro::intersects_aabb(frustum, lo, hi)) {
                    continue; // off-screen tile culled
                }
                draw_mesh(ground, vyro::Mat4::translation({cx, 0.0f, cz}), checker_tex);
                ++tiles_drawn;
            }
        }

        // Level pillars (V6.4): frustum-culled, lit + shadowed.
        for (const auto& o : obstacles) {
            if (vyro::intersects_sphere(frustum, {o.center.x, 1.5f, o.center.z}, 2.6f)) {
                draw_mesh(pillar, pillar_model(o), white_tex);
            }
        }

        // Pickups (V8.2): floating, bobbing, spinning colored cubes.
        {
            const float t = std::chrono::duration<float>(now.time_since_epoch()).count();
            world.view<Position, PickupItem>().for_each([&](Position& pp, PickupItem& item) {
                if (!vyro::intersects_sphere(frustum, {pp.value.x, 0.8f, pp.value.z}, 0.8f)) {
                    return;
                }
                const vyro::f32 bob = 0.8f + 0.15f * std::sin(t * 3.0f);
                const vyro::Mat4 m = vyro::Mat4::translation({pp.value.x, bob, pp.value.z})
                                     * vyro::Mat4::rotation({0, 1, 0}, t * 2.0f);
                draw_mesh(pickup_meshes[static_cast<vyro::usize>(item.kind)], m, white_tex);
            });
        }

        // Co-op tick: publish our soldier and replicate the peer's.
        if (coop) {
            coop->set_local_position({state.player_x, 0.0f, kPlayerZ});
            coop->broadcast();
            coop->poll();
        }
        // Co-op world-state tick (V6.3): host publishes the authoritative
        // score/wave/hp/horde; joiner consumes it.
        if (coop_state) {
            if (coop_is_host) {
                vyro::CoopWorldState ws;
                ws.score = state.score;
                ws.wave = state.wave;
                ws.host_hp = state.hp;
                world.view<Position, EnemyTag>().for_each(
                    [&](Position& p, EnemyTag&) { ws.horde.push_back(p.value); });
                coop_state->send(std::move(ws));
            } else {
                coop_state->poll();
            }
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

        // Animate + draw the horde (V7.1): split across phase buckets so they
        // don't move in lockstep; each bucket is skinned at its own cycle offset
        // and drawn as a separate instanced batch.
        int horde_visible = 0;
        if (!zombie_model.clips.empty()) {
            const float anim_t = std::chrono::duration<float>(now.time_since_epoch()).count();
            constexpr vyro::f32 kBiteNear = 1.6f;
            constexpr vyro::f32 kBiteFar = 3.2f;
            const vyro::f32 bite_weight =
                std::clamp((kBiteFar - nearest) / (kBiteFar - kBiteNear), 0.0f, 1.0f);
            const vyro::f32 walk_dur =
                zombie_model.clips[static_cast<vyro::usize>(walk_clip)].duration;

            for (auto& b : bucket_xforms) {
                b.clear();
            }
            auto push_bucket = [&](vyro::u32 id, const vyro::Mat4& m) {
                const vyro::u32 b = vyro::anim::phase_bucket(id, kPhaseBuckets);
                if (bucket_xforms[b].size() < kMaxZombieInstances) {
                    bucket_xforms[b].push_back(m);
                }
            };

            if (coop_is_joiner && coop_state && coop_state->connected()) {
                // Render the host's authoritative horde (V6.3).
                const auto& horde = coop_state->latest().horde;
                for (vyro::usize i = 0; i < horde.size(); ++i) {
                    const vyro::Vec3& zp = horde[i];
                    if (!vyro::intersects_sphere(frustum, {zp.x, 0.9f, zp.z}, 1.6f)) {
                        continue;
                    }
                    const vyro::f32 yaw = std::atan2(state.player_x - zp.x, kPlayerZ - zp.z);
                    push_bucket(static_cast<vyro::u32>(i),
                                vyro::Mat4::translation(zp)
                                    * vyro::Mat4::rotation({0, 1, 0}, yaw) * zombie_fit);
                }
            } else {
                world.view<Position, EnemyTag>().for_each_entity(
                    [&](vyro::Entity e, Position& p, EnemyTag&) {
                        if (!vyro::intersects_sphere(frustum, {p.value.x, 0.9f, p.value.z}, 1.6f)) {
                            return; // frustum-cull (V5.3)
                        }
                        vyro::f32 yaw = 0.0f;
                        if (const auto* v = world.get_component<Velocity>(e)) {
                            yaw = std::atan2(v->value.x, v->value.z);
                        }
                        // Per-archetype size (brutes bigger, runners smaller) — V7.2.
                        const auto* en = world.get_component<Enemy>(e);
                        const vyro::f32 s = en != nullptr ? kEnemyTypes[en->type].scale : 1.0f;
                        const vyro::Mat4 fit = vyro::Mat4::scale({s, s, s}) * zombie_fit;
                        vyro::Mat4 pose = vyro::Mat4::translation(p.value)
                                          * vyro::Mat4::rotation({0, 1, 0}, yaw) * fit;
                        if (const auto* dying = world.get_component<DyingTag>(e)) {
                            const vyro::f32 k = std::min(dying->t / kDeathDuration, 1.0f);
                            pose = vyro::Mat4::translation({p.value.x, -k * 1.2f, p.value.z})
                                   * vyro::Mat4::rotation({0, 1, 0}, yaw)
                                   * vyro::Mat4::rotation({1, 0, 0}, -k * 1.45f) * fit;
                        }
                        push_bucket(e.index, pose);
                    });
            }

            // Shared instanced-shader state for every bucket draw.
            device.set_uniform_mat4(inst_shader, "u_vp", camera.view_projection());
            device.set_uniform_mat4(inst_shader, "u_lightVP", light_vp);
            device.set_uniform_vec3(inst_shader, "u_lightDir", sun_dir);
            device.bind_texture(device.render_target_texture(shadow_rt), 1);
            device.set_uniform_int(inst_shader, "u_shadowMap", 1);
            device.set_uniform_int(inst_shader, "u_shadowOn", 1);

            // One skinned pose + one instanced draw per non-empty bucket.
            for (vyro::u32 b = 0; b < kPhaseBuckets; ++b) {
                if (bucket_xforms[b].empty()) {
                    continue;
                }
                {
                    VYRO_PROFILE_SCOPE("zombie_skinning");
                    const vyro::f32 t =
                        vyro::anim::bucket_time(b, kPhaseBuckets, walk_dur, anim_t);
                    zombie_model.pose_blend(static_cast<vyro::usize>(walk_clip), t,
                                            static_cast<vyro::usize>(bite_clip), t, bite_weight,
                                            zombie_pose);
                    zombie_model.skin(zombie_pose, zombie_skinned);
                    device.update_buffer(pose_vbos[b], zombie_skinned.data(),
                                         zombie_skinned.size() * sizeof(vyro::Vertex3D));
                }
                device.update_buffer(horde_instance_vbo, bucket_xforms[b].data(),
                                     bucket_xforms[b].size() * sizeof(vyro::Mat4));
                vyro::DrawCommand cmd;
                cmd.shader = inst_shader;
                cmd.vertex_buffer = pose_vbos[b];
                cmd.index_buffer = zombie.ibo;
                cmd.texture = zombie_tex;
                cmd.index_count = zombie.index_count;
                cmd.vertex_format = vyro::VertexFormat::Pos3Normal3UV2;
                device.draw_instanced(cmd, horde_instance_vbo,
                                      static_cast<vyro::u32>(bucket_xforms[b].size()));
                horde_visible += static_cast<int>(bucket_xforms[b].size());
            }
        }
        world.view<Position, BulletTag>().for_each([&](Position& p, BulletTag&) {
            if (!vyro::intersects_sphere(frustum, {p.value.x, 0.9f, p.value.z}, 0.4f)) {
                return;
            }
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
        // The joiner shows the host's authoritative score/wave (V6.3).
        const bool show_host = coop_is_joiner && coop_state && coop_state->connected();
        const int hud_wave = show_host ? coop_state->latest().wave : state.wave;
        const int hud_score = show_host ? coop_state->latest().score : state.score;
        hud_verts.clear();
        vyro::text::build(std::format("WAVE {}/{}", hud_wave, flow.total_waves()), -0.97f, 0.95f,
                          0.07f, hud_aspect, {0.85f, 0.85f, 0.9f}, hud_verts);
        vyro::text::build(std::format("SCORE {}", hud_score), -0.97f, 0.85f, 0.07f,
                          hud_aspect, {1.0f, 0.85f, 0.3f}, hud_verts);
        // Persisted best score (V8.1).
        vyro::text::build(std::format("HIGH {}", std::max(save.high_score, hud_score)), 0.62f,
                          0.85f, 0.05f, hud_aspect, {0.7f, 0.7f, 0.85f}, hud_verts);
        // Objective line (V7.5): kill target, or the intermission countdown.
        if (!coop_is_joiner && !flow.over()) {
            if (flow.phase() == vyro::game::Phase::Intermission) {
                vyro::text::build(std::format("WAVE CLEAR  NEXT IN {}",
                                              static_cast<int>(flow.intermission_left()) + 1),
                                  0.18f, 0.95f, 0.06f, hud_aspect, {0.5f, 0.9f, 0.6f}, hud_verts);
            } else {
                vyro::text::build(std::format("KILL {}/{}", flow.kills(), flow.required()), 0.32f,
                                  0.95f, 0.06f, hud_aspect, {0.9f, 0.6f, 0.5f}, hud_verts);
            }
        }
        vyro::text::build(std::format("TILES {}/{}", tiles_drawn, tiles_total), -0.97f, 0.76f,
                          0.045f, hud_aspect, {0.55f, 0.7f, 0.55f}, hud_verts);
        vyro::text::build(std::format("HORDE {} DRAWS {}", horde_visible,
                                      device.draw_call_count()),
                          -0.97f, 0.70f, 0.045f, hud_aspect, {0.55f, 0.7f, 0.55f}, hud_verts);
        // Weapon + ammo (V7.3).
        {
            const vyro::game::Weapon& wpn = weapons[static_cast<vyro::usize>(current_weapon)];
            const std::string ammo_txt = wpn.reloading()
                                             ? std::string("RELOADING")
                                             : std::format("{}/{}", wpn.ammo(), wpn.stats().mag);
            std::string wname = wpn.stats().name;
            for (char& c : wname) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            vyro::text::build(std::format("{} {}", wname, ammo_txt), -0.97f, -0.86f, 0.06f,
                              hud_aspect, {1.0f, 0.8f, 0.4f}, hud_verts);
        }
        // Difficulty indicator (V8.4): TAB cycles it (applies next run).
        vyro::text::build(std::format("{}  TAB", vyro::game::difficulty_mods(save.difficulty).name),
                          0.66f, -0.92f, 0.045f, hud_aspect, {0.7f, 0.7f, 0.85f}, hud_verts);
        // Profiler readout: FPS / frame time, green under budget, red over.
        const bool over = fps_stats.over_budget(kFrameBudgetMs);
        const vyro::Vec3 perf_col = over ? vyro::Vec3{0.95f, 0.35f, 0.3f}
                                         : vyro::Vec3{0.55f, 0.8f, 0.6f};
        vyro::text::build(std::format("FPS {} {:.1f}MS", static_cast<int>(fps_stats.fps() + 0.5),
                                      fps_stats.average_ms()),
                          -0.97f, 0.64f, 0.045f, hud_aspect, perf_col, hud_verts);
        // Frame-time graph (V6.5): a strip of bars, height = ms vs ~2x budget.
        {
            const vyro::f32 gx0 = 0.62f;
            const vyro::f32 gx1 = 0.97f;
            const vyro::f32 gy0 = 0.62f; // bottom
            const vyro::f32 gh = 0.16f;  // full height
            const vyro::f32 full_ms = static_cast<vyro::f32>(kFrameBudgetMs) * 2.0f;
            const vyro::usize n = frame_history.size();
            for (vyro::usize i = 0; i < n; ++i) {
                const vyro::f32 t0 = gx0 + (gx1 - gx0) * (static_cast<vyro::f32>(i) / kFrameHistory);
                const vyro::f32 t1 =
                    gx0 + (gx1 - gx0) * (static_cast<vyro::f32>(i + 1) / kFrameHistory) - 0.002f;
                const vyro::f32 h = std::clamp(frame_history[i] / full_ms, 0.02f, 1.0f) * gh;
                const vyro::Vec3 c = frame_history[i] > static_cast<vyro::f32>(kFrameBudgetMs)
                                         ? vyro::Vec3{0.95f, 0.35f, 0.3f}
                                         : vyro::Vec3{0.4f, 0.7f, 0.5f};
                const vyro::Vertex3D bl{{t0, gy0, 0}, {}, {}, c};
                const vyro::Vertex3D br{{t1, gy0, 0}, {}, {}, c};
                const vyro::Vertex3D tl{{t0, gy0 + h, 0}, {}, {}, c};
                const vyro::Vertex3D tr{{t1, gy0 + h, 0}, {}, {}, c};
                hud_verts.push_back(bl);
                hud_verts.push_back(br);
                hud_verts.push_back(tr);
                hud_verts.push_back(bl);
                hud_verts.push_back(tr);
                hud_verts.push_back(tl);
            }
        }
        std::string hearts;
        for (int i = 0; i < std::max(state.hp, 0); ++i) {
            hearts += '\x03';
        }
        vyro::text::build(hearts, 0.7f, 0.95f, 0.09f, hud_aspect, {0.95f, 0.2f, 0.25f},
                          hud_verts);
        if (state.game_over) {
            const bool won = flow.phase() == vyro::game::Phase::Victory;
            const char* msg = won ? "VICTORY" : "DEFEAT";
            const char* sub = "PRESS R TO RESTART";
            const vyro::Vec3 col = won ? vyro::Vec3{0.4f, 0.95f, 0.5f} : vyro::Vec3{0.95f, 0.15f, 0.15f};
            const vyro::f32 w1 = vyro::text::measure(msg, 0.22f, hud_aspect);
            vyro::text::build(msg, -w1 * 0.5f, 0.34f, 0.22f, hud_aspect, col, hud_verts);
            // Run summary (V8.3): kills, accuracy, time survived.
            const std::string l1 = std::format("KILLS {}  ACC {}%", stats.kills,
                                                static_cast<int>(stats.accuracy() * 100.0f + 0.5f));
            const std::string l2 = std::format("TIME {}S  SCORE {}",
                                                static_cast<int>(stats.time), state.score);
            const vyro::f32 lw1 = vyro::text::measure(l1, 0.07f, hud_aspect);
            const vyro::f32 lw2 = vyro::text::measure(l2, 0.07f, hud_aspect);
            vyro::text::build(l1, -lw1 * 0.5f, 0.12f, 0.07f, hud_aspect, {0.9f, 0.9f, 0.9f},
                              hud_verts);
            vyro::text::build(l2, -lw2 * 0.5f, 0.02f, 0.07f, hud_aspect, {0.9f, 0.9f, 0.9f},
                              hud_verts);
            const vyro::f32 w2 = vyro::text::measure(sub, 0.07f, hud_aspect);
            vyro::text::build(sub, -w2 * 0.5f, -0.12f, 0.07f, hud_aspect, {0.7f, 0.7f, 0.8f},
                              hud_verts);
        }
        if (hud_verts.size() > kHudMaxVerts) {
            hud_verts.resize(kHudMaxVerts);
        }
        device.update_buffer(hud_vbo, hud_verts.data(),
                             hud_verts.size() * sizeof(vyro::Vertex3D));
        // ── Post-FX pass (V5.1): bloom + tonemap + vignette/flash ────
        // Resolve the offscreen HDR scene to the window with the bloom shader.
        device.bind_render_target({});
        device.set_viewport(window.framebuffer_width(), window.framebuffer_height());
        device.set_depth_test(false);
        device.set_uniform_float(post_shader, "u_flash", damage_flash);
        vyro::DrawCommand post_cmd;
        post_cmd.shader = post_shader;
        post_cmd.vertex_buffer = post_vbo;
        post_cmd.texture = device.render_target_texture(scene_rt);
        post_cmd.vertex_count = static_cast<vyro::u32>(post_verts.size());
        post_cmd.vertex_format = vyro::VertexFormat::Pos3Normal3UV2;
        device.draw(post_cmd);

        // HUD on top so score/wave stay crisp above the post effects.
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
