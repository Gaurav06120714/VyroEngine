// VyroEngine — OpenGL render device implementation
#include "vyro/render/OpenGLDevice.hpp"

#include "vyro/assets/Mesh.hpp"        // Vertex3D layout
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

void set_vertex_attributes(VertexFormat format)
{
    if (format == VertexFormat::Pos3Normal3UV2) {
        constexpr GLsizei stride = static_cast<GLsizei>(sizeof(Vertex3D));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 3));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 6));
        glEnableVertexAttribArray(3); // material color
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 8));
    } else {
        constexpr GLsizei stride = static_cast<GLsizei>(sizeof(QuadVertex));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 3));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 7));
    }
}

} // namespace

OpenGLDevice::OpenGLDevice()
{
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    VYRO_INFO("OpenGL", "device initialized: {}",
              reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

OpenGLDevice::~OpenGLDevice()
{
    for (auto& [id, buf] : m_buffers) {
        glDeleteBuffers(1, &buf.id);
    }
    for (auto& [id, prog] : m_programs) {
        glDeleteProgram(prog);
    }
    for (auto& [id, tex] : m_textures) {
        glDeleteTextures(1, &tex);
    }
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
}

BufferHandle OpenGLDevice::create_buffer(const BufferDesc& desc)
{
    const GLenum target = (desc.type == BufferType::Index) ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindVertexArray(m_vao);
    glBindBuffer(target, buf);
    glBufferData(target, static_cast<GLsizeiptr>(desc.size), desc.data, GL_DYNAMIC_DRAW);
    const u32 id = m_next_id++;
    m_buffers[id] = GLBuffer{buf, target};
    return BufferHandle{id};
}

void OpenGLDevice::update_buffer(BufferHandle handle, const void* data, usize size)
{
    const auto it = m_buffers.find(handle.id);
    if (it == m_buffers.end()) {
        return;
    }
    glBindBuffer(it->second.target, it->second.id);
    glBufferSubData(it->second.target, 0, static_cast<GLsizeiptr>(size), data);
}

void OpenGLDevice::destroy_buffer(BufferHandle handle)
{
    const auto it = m_buffers.find(handle.id);
    if (it != m_buffers.end()) {
        glDeleteBuffers(1, &it->second.id);
        m_buffers.erase(it);
    }
}

TextureHandle OpenGLDevice::create_texture(const TextureDesc& desc)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(desc.width),
                 static_cast<GLsizei>(desc.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, desc.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    const u32 id = m_next_id++;
    m_textures[id] = tex;
    return TextureHandle{id};
}

void OpenGLDevice::destroy_texture(TextureHandle handle)
{
    const auto it = m_textures.find(handle.id);
    if (it != m_textures.end()) {
        glDeleteTextures(1, &it->second);
        m_textures.erase(it);
    }
}

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

RenderTargetHandle OpenGLDevice::create_render_target(const RenderTargetDesc& desc)
{
    const auto w = static_cast<GLsizei>(desc.width);
    const auto h = static_cast<GLsizei>(desc.height);

    // Color texture: clamp + linear, NO mipmaps (an incomplete mip chain would
    // sample as black). Registered in m_textures so the post pass can sample it.
    GLuint color = 0;
    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const GLint internal = desc.hdr ? GL_RGBA16F : GL_RGBA8;
    const GLenum type = desc.hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
    glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, GL_RGBA, type, nullptr);

    GLuint depth = 0;
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        VYRO_ERROR("OpenGL", "render target {}x{} incomplete", desc.width, desc.height);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    const u32 color_id = m_next_id++;
    m_textures[color_id] = color;
    const u32 id = m_next_id++;
    m_render_targets[id] = GLRenderTarget{fbo, color_id, depth, desc.width, desc.height};
    return RenderTargetHandle{id};
}

void OpenGLDevice::destroy_render_target(RenderTargetHandle handle)
{
    const auto it = m_render_targets.find(handle.id);
    if (it == m_render_targets.end()) {
        return;
    }
    glDeleteFramebuffers(1, &it->second.fbo);
    glDeleteRenderbuffers(1, &it->second.depth_rbo);
    destroy_texture(TextureHandle{it->second.color_tex_id});
    m_render_targets.erase(it);
}

void OpenGLDevice::bind_render_target(RenderTargetHandle handle)
{
    const auto it = m_render_targets.find(handle.id);
    if (it == m_render_targets.end()) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to the window
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
    glViewport(0, 0, static_cast<GLsizei>(it->second.width),
               static_cast<GLsizei>(it->second.height));
}

TextureHandle OpenGLDevice::render_target_texture(RenderTargetHandle handle)
{
    const auto it = m_render_targets.find(handle.id);
    return it != m_render_targets.end() ? TextureHandle{it->second.color_tex_id} : TextureHandle{};
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

    ++m_draw_calls;
    glUseProgram(prog->second);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo->second.id);
    set_vertex_attributes(command.vertex_format);

    if (command.texture.valid()) {
        const auto tex = m_textures.find(command.texture.id);
        if (tex != m_textures.end()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex->second);
        }
    }

    if (command.index_buffer.valid()) {
        const auto ibo = m_buffers.find(command.index_buffer.id);
        if (ibo != m_buffers.end()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo->second.id);
            glDrawElements(to_gl_topology(command.topology),
                           static_cast<GLsizei>(command.index_count), GL_UNSIGNED_INT, nullptr);
            return;
        }
    }
    glDrawArrays(to_gl_topology(command.topology), 0, static_cast<GLsizei>(command.vertex_count));
}

void OpenGLDevice::end_frame() {}

void OpenGLDevice::set_depth_test(bool enabled)
{
    if (enabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void OpenGLDevice::set_uniform_mat4(ShaderHandle shader, const char* name, const Mat4& value)
{
    const auto it = m_programs.find(shader.id);
    if (it == m_programs.end()) {
        return;
    }
    glUseProgram(it->second);
    const GLint loc = glGetUniformLocation(it->second, name);
    if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, value.data.data());
    }
}

void OpenGLDevice::set_uniform_vec3(ShaderHandle shader, const char* name, Vec3 value)
{
    const auto it = m_programs.find(shader.id);
    if (it == m_programs.end()) {
        return;
    }
    glUseProgram(it->second);
    const GLint loc = glGetUniformLocation(it->second, name);
    if (loc >= 0) {
        glUniform3f(loc, value.x, value.y, value.z);
    }
}

void OpenGLDevice::set_uniform_float(ShaderHandle shader, const char* name, f32 value)
{
    const auto it = m_programs.find(shader.id);
    if (it == m_programs.end()) {
        return;
    }
    glUseProgram(it->second);
    const GLint loc = glGetUniformLocation(it->second, name);
    if (loc >= 0) {
        glUniform1f(loc, value);
    }
}

void OpenGLDevice::set_uniform_int(ShaderHandle shader, const char* name, i32 value)
{
    const auto it = m_programs.find(shader.id);
    if (it == m_programs.end()) {
        return;
    }
    glUseProgram(it->second);
    const GLint loc = glGetUniformLocation(it->second, name);
    if (loc >= 0) {
        glUniform1i(loc, value);
    }
}

void OpenGLDevice::bind_texture(TextureHandle handle, u32 unit)
{
    const auto it = m_textures.find(handle.id);
    if (it == m_textures.end()) {
        return;
    }
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, it->second);
    glActiveTexture(GL_TEXTURE0);
}

} // namespace vyro
