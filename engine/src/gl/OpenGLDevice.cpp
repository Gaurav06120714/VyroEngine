// VyroEngine — OpenGL render device implementation
#include "vyro/render/OpenGLDevice.hpp"

#include "vyro/core/Log.hpp"
#include "vyro/render/Renderer2D.hpp" // QuadVertex layout

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>

namespace vyro {

namespace {

GLenum to_gl_topology(PrimitiveTopology t)
{
    switch (t) {
        case PrimitiveTopology::Triangles: return GL_TRIANGLES;
        case PrimitiveTopology::Lines: return GL_LINES;
        case PrimitiveTopology::Points: return GL_POINTS;
    }
    return GL_TRIANGLES;
}

GLuint compile(GLenum stage, const char* source)
{
    const GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        VYRO_ERROR("OpenGL", "shader compile failed: {}", log);
    }
    return shader;
}

} // namespace

OpenGLDevice::OpenGLDevice()
{
    // A bound VAO is required to draw in the core profile.
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    VYRO_INFO("OpenGL", "device initialized: {}",
              reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

OpenGLDevice::~OpenGLDevice()
{
    for (auto& [id, buf] : m_buffers) {
        glDeleteBuffers(1, &buf);
    }
    for (auto& [id, prog] : m_programs) {
        glDeleteProgram(prog);
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
}

BufferHandle OpenGLDevice::create_buffer(const BufferDesc& desc)
{
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(desc.size), desc.data, GL_DYNAMIC_DRAW);
    const u32 id = m_next_id++;
    m_buffers[id] = buf;
    return BufferHandle{id};
}

void OpenGLDevice::update_buffer(BufferHandle handle, const void* data, usize size)
{
    const auto it = m_buffers.find(handle.id);
    if (it == m_buffers.end()) {
        return;
    }
    glBindBuffer(GL_ARRAY_BUFFER, it->second);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(size), data);
}

void OpenGLDevice::destroy_buffer(BufferHandle handle)
{
    const auto it = m_buffers.find(handle.id);
    if (it != m_buffers.end()) {
        glDeleteBuffers(1, &it->second);
        m_buffers.erase(it);
    }
}

TextureHandle OpenGLDevice::create_texture(const TextureDesc& /*desc*/)
{
    return TextureHandle{m_next_id++}; // texture upload not needed for this demo
}

void OpenGLDevice::destroy_texture(TextureHandle /*handle*/) {}

ShaderHandle OpenGLDevice::create_shader(const ShaderDesc& desc)
{
    const GLuint vs = compile(GL_VERTEX_SHADER, desc.vertex_source);
    const GLuint fs = compile(GL_FRAGMENT_SHADER, desc.fragment_source);
    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    const u32 id = m_next_id++;
    m_programs[id] = program;
    return ShaderHandle{id};
}

void OpenGLDevice::destroy_shader(ShaderHandle handle)
{
    const auto it = m_programs.find(handle.id);
    if (it != m_programs.end()) {
        glDeleteProgram(it->second);
        m_programs.erase(it);
    }
}

void OpenGLDevice::begin_frame() {}

void OpenGLDevice::set_viewport(u32 width, u32 height)
{
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void OpenGLDevice::clear(Vec4 color)
{
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLDevice::draw(const DrawCommand& command)
{
    const auto prog = m_programs.find(command.shader.id);
    const auto vbo = m_buffers.find(command.vertex_buffer.id);
    if (prog == m_programs.end() || vbo == m_buffers.end()) {
        return;
    }

    glUseProgram(prog->second);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->second);

    // Interleaved QuadVertex: position(vec3), color(vec4), uv(vec2).
    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(QuadVertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 3));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 7));

    glDrawArrays(to_gl_topology(command.topology), 0, static_cast<GLsizei>(command.vertex_count));
}

void OpenGLDevice::end_frame() {}

} // namespace vyro
