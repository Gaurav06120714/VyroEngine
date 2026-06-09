# VyroEngine Rendering Rules

**Version:** 1.0.0 | **Inspired by:** Godot RenderingServer, Unreal RHI

---

## Architecture Rules

### Rule R-1: All Rendering Through the GAL
Game code and engine systems never call OpenGL/Vulkan/Metal APIs directly. All calls go through `IRenderDevice` or higher-level renderer APIs.

```cpp
// WRONG: raw API call
glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr); // NO

// CORRECT: through GAL
m_device->draw_indexed(draw_cmd);
```

### Rule R-2: Render State is Explicit
Never rely on "current" OpenGL state. Always set the state you need. Treat GPU state as if it is unknown at the start of every pass.

### Rule R-3: Render Thread Ownership
All GPU resource creation, destruction, and command submission happens on the render thread. Main thread builds a render command list; render thread executes it.

### Rule R-4: Render Graph is the Only Pipeline
All rendering passes must be registered in the `RenderGraph`. No ad-hoc pass creation. The graph handles:
- Resource creation/destruction
- GPU resource transitions (Vulkan barriers)
- Pass ordering and dependency

### Rule R-5: No Per-Frame Heap Allocations
The render loop must not allocate from the heap. Use the frame stack allocator for temporary render data.

---

## Resource Rules

### Rule RES-1: All Resources are Handle-Based
```cpp
// CORRECT
BufferHandle    vbo  = m_device->create_buffer(desc);
TextureHandle   tex  = m_device->create_texture(desc);
PipelineHandle  pipe = m_device->create_pipeline(desc);

// WRONG: pointer to GPU resource
GLuint vbo; // raw GL handle leaks, not tracked
```

### Rule RES-2: Resources are Immutable After Creation
After creating a buffer or texture with a given size/format, that size/format never changes. Create a new resource instead.

Exception: buffer data upload (contents can change, descriptor cannot).

### Rule RES-3: Staging Buffer Protocol
For GPU-only resources (DEVICE_LOCAL), always use a staging buffer for uploads:
1. Create `STAGING` buffer (host-visible)
2. Write data to staging
3. Issue copy command: `staging → device-local`
4. Destroy staging buffer after copy

### Rule RES-4: Descriptor Sets are Pooled
Never create descriptor sets individually per-draw. Use a descriptor pool with set caching. Reuse sets with the same layout.

---

## Shader Rules

### Rule S-1: GLSL Only
All shaders are written in GLSL 4.60. No HLSL. Vulkan backend compiles GLSL to SPIR-V via glslc.

### Rule S-2: Shader Includes
Use the `#include` system for shared shader code. Never duplicate shader snippets.

```glsl
// Common shared code lives in:
// shaders/common/math.glsl
// shaders/common/pbr.glsl
// shaders/common/shadows.glsl
// shaders/common/lighting.glsl

#include "common/pbr.glsl"
```

### Rule S-3: Uniform Buffer Layout
All shader uniforms must use `std140` or `std430` layout. No naked uniforms in production shaders.

```glsl
layout(std140, binding = 0) uniform FrameData {
    mat4 view;
    mat4 projection;
    vec3 camera_pos;
    float time;
};
```

### Rule S-4: Shader Hot Reload (Debug Only)
Hot reload is a debug-only feature. Shaders are pre-compiled to SPIR-V in release builds. Never ship GLSL source in release packages.

### Rule S-5: Shader Naming Convention
```
shaders/
├── [pass]/[name].[stage]
examples:
├── geometry/mesh.vert
├── geometry/mesh.frag
├── shadows/shadow_map.vert
├── postprocess/bloom.frag
├── compute/particle_update.comp
```

---

## Performance Rules

### Rule P-1: Sort Draw Calls
Sort by: (1) render layer, (2) shader, (3) material, (4) mesh. Minimizes GPU state changes.

### Rule P-2: Instance Everything Repeating
Any mesh drawn more than once per frame must use GPU instancing.

### Rule P-3: Frustum Cull on CPU
Never submit out-of-frustum objects to the GPU. Frustum culling runs on the CPU before building the draw call list.

### Rule P-4: LOD Required for Complex Meshes
Any mesh above 10,000 triangles must have at least 2 LOD levels. Define LOD transitions in the `MeshComponent`.

### Rule P-5: Texture Format Policy

| Content | Format |
|---------|--------|
| Albedo (sRGB) | BC3 / BC7 (compressed) |
| Normal map | BC5 (2-channel compressed) |
| Roughness/Metallic | BC4 / BC5 |
| HDR skybox | RGBA16F |
| UI / sprites | RGBA8 (uncompressed acceptable) |
| Depth buffer | D32F or D24S8 |

---

## 2D Renderer Rules

### Rule 2D-1: Always Use the Sprite Batcher
Never draw a single quad with a separate draw call. Always push to `Renderer2D::draw_sprite()` and let the batcher flush.

### Rule 2D-2: Atlas All Small Textures
Any texture under 512×512 that is used frequently should be in a texture atlas. Reduces texture binding switches.

### Rule 2D-3: Z-Order Not Depth Buffer
2D rendering uses explicit z-order sorting, not the depth buffer. Set `z_order` on sprites to control layering.

---

## Vulkan-Specific Rules

### Rule VK-1: Validation Layers Always On in Debug
```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(vyro_renderer PRIVATE VYRO_VULKAN_VALIDATION=1)
endif()
```

### Rule VK-2: No Layout Undefined Transitions
Always specify exact initial and final layouts in image barriers. Never use `VK_IMAGE_LAYOUT_UNDEFINED` for the final layout.

### Rule VK-3: Pipeline Barriers are Explicit
Document every barrier with: what resource, what stage, what access, why.

```cpp
// Transition shadow map from depth-write to shader-read
// WHY: shadow pass writes depth; lighting pass reads it as texture
transition_image(shadow_depth,
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,   // src stage
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,        // dst stage
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, // src access
    VK_ACCESS_SHADER_READ_BIT,                    // dst access
    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
```

### Rule VK-4: Destroy in Reverse Creation Order
Vulkan resources must be destroyed in reverse creation order. The render device destructor must handle this correctly.
