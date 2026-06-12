// VyroEditor — the visual editor application (V2.1)
// Dear ImGui panels driven by the engine's headless editor models:
//   Hierarchy  -> EditorContext selection + entity list
//   Inspector  -> Inspector property reflection on the selected entity
//   Assets     -> AssetBrowser scan of the project assets/ folder
//   Stats      -> Profiler frame statistics
// The 3D viewport renders the active model with the OpenGL backend behind
// the UI, exactly like the standalone viewer.
#include "vyro/assets/Mesh.hpp"
#include "vyro/assets/ObjLoader.hpp"
#include "vyro/core/Engine.hpp"
#include "vyro/core/Log.hpp"
#include "vyro/core/Profiler.hpp"
#include "vyro/ecs/Registry.hpp"
#include "vyro/editor/AssetBrowser.hpp"
#include "vyro/editor/EditorContext.hpp"
#include "vyro/editor/Gizmo.hpp"
#include "vyro/scene/SceneSerializer.hpp"

#include "vyro/math/Mat4.hpp"
#include "vyro/platform/Window.hpp"
#include "vyro/render/Camera.hpp"
#include "vyro/render/OpenGLDevice.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <GLFW/glfw3.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

namespace {

struct NameComponent {
    std::string name;
};

vyro::MeshData load_model()
{
    for (const char* path : {"assets/models/model.obj", "../assets/models/model.obj",
                             "assets/models/cube.obj", "../assets/models/cube.obj"}) {
        auto result = vyro::load_obj(path);
        if (result.has_value()) {
            VYRO_INFO("Editor", "loaded model '{}'", path);
            return std::move(result.value());
        }
    }
    return vyro::make_cube(2.0f);
}

vyro::Mat4 fit_transform(const vyro::MeshData& mesh)
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
    const vyro::Vec3 center{(lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f, (lo.z + hi.z) * 0.5f};
    vyro::f32 extent = std::max({hi.x - lo.x, hi.y - lo.y, hi.z - lo.z});
    const vyro::f32 s = extent > 0.0f ? 2.0f / extent : 1.0f;
    return vyro::Mat4::scale({s, s, s}) * vyro::Mat4::translation(-center);
}

} // namespace

int main()
{
    vyro::Engine::print_banner();

    vyro::Window window(1440, 900, "VyroEditor");
    if (!window.valid()) {
        VYRO_ERROR("Editor", "no display available");
        return 1;
    }

    // ── ImGui setup ──────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.native(), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // ── Scene: a few entities with transforms ────────────────────────
    vyro::Registry registry;
    vyro::EditorContext editor(registry);

    const char* names[] = {"Spaceship", "Camera Rig", "Sun Light"};
    std::vector<vyro::Entity> entities;
    for (const char* n : names) {
        const auto e = registry.create_entity();
        registry.add_component<NameComponent>(e, NameComponent{n});
        registry.add_component<vyro::TransformComponent>(e, vyro::TransformComponent{});
        entities.push_back(e);
    }
    editor.select(entities[0]);

    // ── Scene authoring (V3.5): schema + save/load path ──────────────
    vyro::SceneSchema schema;
    schema.register_component<NameComponent>(
        "Name",
        [](const NameComponent& n) { return vyro::FieldMap{{"name", n.name}}; },
        [](const vyro::FieldMap& f) { return NameComponent{f.at("name")}; });
    schema.register_component<vyro::TransformComponent>(
        "Transform",
        [](const vyro::TransformComponent& t) {
            auto v = [](vyro::f32 x) { return std::to_string(x); };
            return vyro::FieldMap{{"px", v(t.position.x)}, {"py", v(t.position.y)},
                                  {"pz", v(t.position.z)}, {"rx", v(t.rotation.x)},
                                  {"ry", v(t.rotation.y)}, {"rz", v(t.rotation.z)},
                                  {"sx", v(t.scale.x)},    {"sy", v(t.scale.y)},
                                  {"sz", v(t.scale.z)}};
        },
        [](const vyro::FieldMap& f) {
            auto g = [&](const char* k) { return std::stof(f.at(k)); };
            vyro::TransformComponent t;
            t.position = {g("px"), g("py"), g("pz")};
            t.rotation = {g("rx"), g("ry"), g("rz")};
            t.scale = {g("sx"), g("sy"), g("sz")};
            return t;
        });
    const char* scene_path = "assets/scenes/editor.scene";
    std::filesystem::create_directories("assets/scenes");
    int next_entity_number = static_cast<int>(entities.size()) + 1;

    // ── Assets panel data ────────────────────────────────────────────
    vyro::AssetBrowser assets;
    if (assets.scan("assets") == 0) {
        assets.scan("../assets");
    }

    // ── Viewport renderer (same pipeline as vyro_model) ──────────────
    vyro::OpenGLDevice device;
    const vyro::MeshData mesh = load_model();

    vyro::BufferDesc vbd;
    vbd.type = vyro::BufferType::Vertex;
    vbd.size = mesh.vertex_bytes();
    vbd.data = mesh.vertices.data();
    const auto vbo = device.create_buffer(vbd);
    vyro::BufferDesc ibd;
    ibd.type = vyro::BufferType::Index;
    ibd.size = mesh.index_bytes();
    ibd.data = mesh.indices.data();
    const auto ibo = device.create_buffer(ibd);

    const char* vs = R"(#version 330 core
layout(location=0) in vec3 aPos; layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;  layout(location=3) in vec3 aColor;
uniform mat4 u_mvp; uniform mat4 u_model;
out vec3 vNormal; out vec3 vColor;
void main(){ gl_Position=u_mvp*vec4(aPos,1.0); vNormal=mat3(u_model)*aNormal; vColor=aColor; })";
    const char* fs = R"(#version 330 core
in vec3 vNormal; in vec3 vColor; out vec4 FragColor; uniform vec3 u_lightDir;
void main(){ float d=max(dot(normalize(vNormal),normalize(-u_lightDir)),0.0);
FragColor=vec4(vColor*(0.35+0.65*d),1.0); })";
    const auto shader = device.create_shader({vs, fs});

    vyro::Camera camera;
    const vyro::f32 aspect = static_cast<vyro::f32>(window.framebuffer_width())
                             / static_cast<vyro::f32>(window.framebuffer_height());
    camera.set_perspective(1.0472f, aspect, 0.1f, 100.0f);
    camera.set_view({0.0f, 1.2f, 4.0f}, {0.0f, 0.0f, 0.0f});

    const vyro::Mat4 fit = fit_transform(mesh);
    const auto start = std::chrono::steady_clock::now();
    auto last_frame = start;

    VYRO_INFO("Editor", "VyroEditor open. Close the window to quit.");

    while (window.is_open()) {
        const auto now = std::chrono::steady_clock::now();
        const float t = std::chrono::duration<float>(now - start).count();
        const double frame_ms = std::chrono::duration<double, std::milli>(now - last_frame).count();
        last_frame = now;
        vyro::Profiler::instance().end_frame(frame_ms);

        window.poll_events();

        // ── 3D viewport (background) ─────────────────────────────────
        {
            VYRO_PROFILE_SCOPE("Viewport");
            auto* selected_tx =
                editor.has_selection()
                    ? registry.get_component<vyro::TransformComponent>(editor.selected())
                    : nullptr;
            const vyro::Vec3 offset = selected_tx != nullptr ? selected_tx->position : vyro::Vec3{};
            const vyro::Mat4 model = vyro::Mat4::translation(offset)
                                     * vyro::Mat4::rotation({0, 1, 0}, t * 0.6f)
                                     * vyro::Mat4::rotation({1, 0, 0}, 0.4f) * fit;

            device.set_viewport(window.framebuffer_width(), window.framebuffer_height());
            device.clear({0.06f, 0.07f, 0.10f, 1.0f});
            device.set_uniform_mat4(shader, "u_mvp", camera.view_projection() * model);
            device.set_uniform_mat4(shader, "u_model", model);
            device.set_uniform_vec3(shader, "u_lightDir", {-0.4f, -1.0f, -0.6f});

            vyro::DrawCommand cmd;
            cmd.shader = shader;
            cmd.vertex_buffer = vbo;
            cmd.index_buffer = ibo;
            cmd.index_count = static_cast<vyro::u32>(mesh.indices.size());
            cmd.vertex_format = vyro::VertexFormat::Pos3Normal3UV2;
            device.draw(cmd);
        }

        // ── ImGui panels ─────────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Hierarchy
        ImGui::SetNextWindowPos({10, 10}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({260, 280}, ImGuiCond_FirstUseEver);
        ImGui::Begin("Hierarchy");
        if (ImGui::Button("+ Entity")) {
            const auto e = registry.create_entity();
            registry.add_component<NameComponent>(
                e, NameComponent{std::format("Entity {}", next_entity_number++)});
            registry.add_component<vyro::TransformComponent>(e, vyro::TransformComponent{});
            editor.select(e);
        }
        ImGui::SameLine();
        if (ImGui::Button("- Delete") && editor.has_selection()) {
            registry.destroy_entity(editor.selected());
            editor.clear_selection();
        }
        if (ImGui::Button("Save Scene")) {
            vyro::scene_io::save_file(scene_path, registry, schema);
            VYRO_INFO("Editor", "scene saved to {}", scene_path);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Scene")) {
            for (const auto e : registry.alive_entities()) {
                registry.destroy_entity(e);
            }
            editor.clear_selection();
            const auto loaded = vyro::scene_io::load_file(scene_path, registry, schema);
            VYRO_INFO("Editor", "scene load: {} entities",
                      loaded.has_value() ? static_cast<int>(loaded.value()) : -1);
        }
        ImGui::Separator();
        for (const auto e : registry.alive_entities()) {
            const auto* name = registry.get_component<NameComponent>(e);
            const bool selected = editor.has_selection() && editor.selected() == e;
            ImGui::PushID(static_cast<int>(e.index));
            if (ImGui::Selectable(name != nullptr ? name->name.c_str() : "Entity", selected)) {
                editor.select(e);
            }
            ImGui::PopID();
        }
        ImGui::End();

        // Inspector
        ImGui::SetNextWindowPos({10, 300}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({260, 300}, ImGuiCond_FirstUseEver);
        ImGui::Begin("Inspector");
        if (editor.has_selection()) {
            auto* name = registry.get_component<NameComponent>(editor.selected());
            auto* tx = registry.get_component<vyro::TransformComponent>(editor.selected());
            if (name != nullptr) {
                ImGui::Text("%s", name->name.c_str());
                ImGui::Separator();
            }
            if (tx != nullptr) {
                ImGui::DragFloat3("Position", &tx->position.x, 0.05f);
                ImGui::DragFloat3("Rotation", &tx->rotation.x, 0.02f);
                ImGui::DragFloat3("Scale", &tx->scale.x, 0.02f, 0.01f, 50.0f);
            }
        } else {
            ImGui::TextDisabled("Nothing selected");
        }
        ImGui::End();

        // Assets
        ImGui::SetNextWindowPos({10, 610}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({260, 240}, ImGuiCond_FirstUseEver);
        ImGui::Begin("Assets");
        for (const auto& entry : assets.entries()) {
            ImGui::BulletText("%s", entry.name.c_str());
        }
        if (assets.entries().empty()) {
            ImGui::TextDisabled("(no assets found)");
        }
        ImGui::End();

        // Stats
        ImGui::SetNextWindowPos({1170, 10}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize({260, 140}, ImGuiCond_FirstUseEver);
        ImGui::Begin("Stats");
        ImGui::Text("Frame: %.2f ms", vyro::Profiler::instance().average_frame_ms());
        ImGui::Text("Peak:  %.2f ms", vyro::Profiler::instance().peak_frame_ms());
        ImGui::Text("Frames: %llu",
                    static_cast<unsigned long long>(vyro::Profiler::instance().frame_count()));
        ImGui::Text("Entities: %zu", registry.entity_count());
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.swap_buffers();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    VYRO_INFO("Editor", "VyroEditor closed.");
    return 0;
}
