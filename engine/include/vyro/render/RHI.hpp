// VyroEngine — Render Hardware Interface (RHI)
// Phase 3.1: a thin abstraction over the GPU. Higher layers (renderer, 2D/3D)
// speak only RHI; concrete backends (Null today; Vulkan/OpenGL later) implement
// IRenderDevice. This isolation is the engine's primary rendering-risk
// mitigation — gameplay code never touches a VkDevice.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/math/Vec.hpp"

namespace vyro {

enum class RenderBackend : u8 {
    Null,
    OpenGL,
    Vulkan,
};

enum class BufferType : u8 {
    Vertex,
    Index,
    Uniform,
};

enum class TextureFormat : u8 {
    R8,
    RGB8,
    RGBA8,
    Depth24Stencil8,
};

enum class ShaderStage : u8 {
    Vertex,
    Fragment,
};

enum class PrimitiveTopology : u8 {
    Triangles,
    Lines,
    Points,
};

// ── Opaque resource handles (0 == invalid) ───────────────────────────
struct BufferHandle {
    u32 id = 0;
    [[nodiscard]] bool valid() const { return id != 0; }
    friend bool operator==(BufferHandle a, BufferHandle b) { return a.id == b.id; }
};
struct TextureHandle {
    u32 id = 0;
    [[nodiscard]] bool valid() const { return id != 0; }
    friend bool operator==(TextureHandle a, TextureHandle b) { return a.id == b.id; }
};
struct ShaderHandle {
    u32 id = 0;
    [[nodiscard]] bool valid() const { return id != 0; }
    friend bool operator==(ShaderHandle a, ShaderHandle b) { return a.id == b.id; }
};

// ── Resource descriptors ─────────────────────────────────────────────
struct BufferDesc {
    BufferType type = BufferType::Vertex;
    usize size = 0;            // bytes
    const void* data = nullptr; // optional initial data
};

struct TextureDesc {
    u32 width = 0;
    u32 height = 0;
    TextureFormat format = TextureFormat::RGBA8;
    const void* pixels = nullptr;
};

struct ShaderDesc {
    const char* vertex_source = nullptr;
    const char* fragment_source = nullptr;
};

// A single draw submission.
struct DrawCommand {
    ShaderHandle shader;
    BufferHandle vertex_buffer;
    BufferHandle index_buffer; // optional; if invalid, draws vertex_count verts
    u32 index_count = 0;
    u32 vertex_count = 0;
    PrimitiveTopology topology = PrimitiveTopology::Triangles;
};

// ── Device interface ─────────────────────────────────────────────────
class IRenderDevice
{
public:
    virtual ~IRenderDevice() = default;

    [[nodiscard]] virtual RenderBackend backend() const = 0;

    // Resource lifetime.
    [[nodiscard]] virtual BufferHandle create_buffer(const BufferDesc& desc) = 0;
    virtual void update_buffer(BufferHandle handle, const void* data, usize size) = 0;
    virtual void destroy_buffer(BufferHandle handle) = 0;
    [[nodiscard]] virtual TextureHandle create_texture(const TextureDesc& desc) = 0;
    virtual void destroy_texture(TextureHandle handle) = 0;
    [[nodiscard]] virtual ShaderHandle create_shader(const ShaderDesc& desc) = 0;
    virtual void destroy_shader(ShaderHandle handle) = 0;

    // Frame and draw.
    virtual void begin_frame() = 0;
    virtual void set_viewport(u32 width, u32 height) = 0;
    virtual void clear(Vec4 color) = 0;
    virtual void draw(const DrawCommand& command) = 0;
    virtual void end_frame() = 0;
};

} // namespace vyro
