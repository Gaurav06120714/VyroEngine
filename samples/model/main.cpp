// VyroEngine — 3D model viewer
// Opens a window and renders a 3D model (loaded from a .obj file) with the
// OpenGL backend: perspective camera, depth testing, and directional lighting.
// Auto-centers and scales any model to fit the view. Falls back to the bundled
// cube, then a procedural cube, if no model file is found.
#include "vyro/assets/Mesh.hpp"
#include "vyro/assets/ObjLoader.hpp"
#include "vyro/core/Engine.hpp"
#include "vyro/core/Log.hpp"
#include "vyro/math/Mat4.hpp"
#include "vyro/platform/Window.hpp"
#include "vyro/render/Camera.hpp"
#include "vyro/render/OpenGLDevice.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace {

// Load the model from argv, then assets/models/model.obj, then cube.obj, else
// a procedural cube.
vyro::MeshData load_model(int argc, char** argv)
{
    std::vector<std::string> candidates;
    if (argc > 1) {
        candidates.emplace_back(argv[1]);
    }
    candidates.emplace_back("assets/models/model.obj");
    candidates.emplace_back("../assets/models/model.obj");
    candidates.emplace_back("assets/models/cube.obj");
    candidates.emplace_back("../assets/models/cube.obj");

    for (const std::string& path : candidates) {
        auto result = vyro::load_obj(path);
        if (result.has_value()) {
            VYRO_INFO("Viewer", "loaded model '{}' ({} verts, {} indices)", path,
                      result->vertices.size(), result->indices.size());
            return std::move(result.value());
        }
    }
    VYRO_WARN("Viewer", "no .obj found; using procedural cube");
    return vyro::make_cube(2.0f);
}

// Center-and-fit transform so any model fills the view regardless of its scale.
vyro::Mat4 fit_transform(const vyro::MeshData& mesh)
{
    vyro::Vec3 lo = mesh.vertices[0].position;
    vyro::Vec3 hi = mesh.vertices[0].position;
    for (const auto& v : mesh.vertices) {
        lo.x = v.position.x < lo.x ? v.position.x : lo.x;
        lo.y = v.position.y < lo.y ? v.position.y : lo.y;
        lo.z = v.position.z < lo.z ? v.position.z : lo.z;
        hi.x = v.position.x > hi.x ? v.position.x : hi.x;
        hi.y = v.position.y > hi.y ? v.position.y : hi.y;
        hi.z = v.position.z > hi.z ? v.position.z : hi.z;
    }
    const vyro::Vec3 center{(lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f, (lo.z + hi.z) * 0.5f};
    const vyro::f32 ex = hi.x - lo.x;
    const vyro::f32 ey = hi.y - lo.y;
    const vyro::f32 ez = hi.z - lo.z;
    vyro::f32 extent = ex > ey ? ex : ey;
    extent = ez > extent ? ez : extent;
    const vyro::f32 s = extent > 0.0f ? 2.0f / extent : 1.0f;
    return vyro::Mat4::scale(vyro::Vec3{s, s, s}) * vyro::Mat4::translation(vyro::Vec3{-center.x, -center.y, -center.z});
}

} // namespace

int main(int argc, char** argv)
{
    vyro::Engine::print_banner();

    vyro::Window window(1024, 720, "VyroEngine - 3D Model Viewer");
    if (!window.valid()) {
        VYRO_ERROR("Viewer", "no window/display available");
        return 1;
    }

    vyro::OpenGLDevice device;
    const vyro::MeshData mesh = load_model(argc, argv);

    // Upload geometry.
    vyro::BufferDesc vbd;
    vbd.type = vyro::BufferType::Vertex;
    vbd.size = mesh.vertex_bytes();
    vbd.data = mesh.vertices.data();
    const vyro::BufferHandle vbo = device.create_buffer(vbd);

    vyro::BufferDesc ibd;
    ibd.type = vyro::BufferType::Index;
    ibd.size = mesh.index_bytes();
    ibd.data = mesh.indices.data();
    const vyro::BufferHandle ibo = device.create_buffer(ibd);

    const char* vs = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aColor;
uniform mat4 u_mvp;
uniform mat4 u_model;
out vec3 vNormal;
out vec3 vColor;
void main() {
    gl_Position = u_mvp * vec4(aPos, 1.0);
    vNormal = mat3(u_model) * aNormal;
    vColor = aColor;
})";

    const char* fs = R"(#version 330 core
in vec3 vNormal;
in vec3 vColor;
out vec4 FragColor;
uniform vec3 u_lightDir;
void main() {
    float d = max(dot(normalize(vNormal), normalize(-u_lightDir)), 0.0);
    vec3 c = vColor * (0.35 + 0.65 * d);
    FragColor = vec4(c, 1.0);
})";

    const vyro::ShaderHandle shader = device.create_shader({vs, fs});

    const vyro::f32 aspect =
        static_cast<vyro::f32>(window.framebuffer_width()) / static_cast<vyro::f32>(window.framebuffer_height());
    vyro::Camera camera;
    camera.set_perspective(1.0472f /*60deg*/, aspect, 0.1f, 100.0f);
    camera.set_view(vyro::Vec3{0.0f, 1.2f, 4.0f}, vyro::Vec3{0.0f, 0.0f, 0.0f});

    const vyro::Mat4 fit = fit_transform(mesh);
    const auto start = std::chrono::steady_clock::now();

    VYRO_INFO("Viewer", "Window open. Close it to quit.");

    while (window.is_open()) {
        const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();

        // Spin the model around Y, with a slight fixed tilt to show depth.
        const vyro::Mat4 spin = vyro::Mat4::rotation(vyro::Vec3{0, 1, 0}, t * 0.8f);
        const vyro::Mat4 tilt = vyro::Mat4::rotation(vyro::Vec3{1, 0, 0}, 0.5f);
        const vyro::Mat4 model = spin * tilt * fit;
        const vyro::Mat4 mvp = camera.view_projection() * model;

        device.set_viewport(window.framebuffer_width(), window.framebuffer_height());
        device.clear(vyro::Vec4{0.06f, 0.07f, 0.10f, 1.0f});

        device.set_uniform_mat4(shader, "u_mvp", mvp);
        device.set_uniform_mat4(shader, "u_model", model);
        device.set_uniform_vec3(shader, "u_lightDir", vyro::Vec3{-0.4f, -1.0f, -0.6f});

        vyro::DrawCommand cmd;
        cmd.shader = shader;
        cmd.vertex_buffer = vbo;
        cmd.index_buffer = ibo;
        cmd.index_count = static_cast<vyro::u32>(mesh.indices.size());
        cmd.vertex_format = vyro::VertexFormat::Pos3Normal3UV2;
        cmd.topology = vyro::PrimitiveTopology::Triangles;
        device.draw(cmd);

        window.swap_buffers();
        window.poll_events();
    }

    VYRO_INFO("Viewer", "Window closed. Goodbye.");
    return 0;
}
