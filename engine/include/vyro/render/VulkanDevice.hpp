// VyroEngine — Vulkan render device (V2.4, stage a)
// A real Vulkan backend via MoltenVK on macOS implementing the RHI resource
// surface: instance + physical/logical device init and host-visible buffer
// allocation with upload and readback. Swapchain presentation (stage b) layers
// on top; the FrameGraph already provides the pass scheduling it will record.
#pragma once

#include "vyro/render/RHI.hpp"

#include <string>
#include <unordered_map>

// Vulkan handles are opaque pointers/uint64s; forward-declare to keep the
// header free of vulkan.h for engine clients.
struct VkInstance_T;
struct VkPhysicalDevice_T;
struct VkDevice_T;
struct VkQueue_T;

namespace vyro {

class VulkanDevice final : public IRenderDevice
{
public:
    VulkanDevice();
    ~VulkanDevice() override;

    // True when instance + device creation succeeded.
    [[nodiscard]] bool valid() const { return m_device != nullptr; }
    [[nodiscard]] const std::string& adapter_name() const { return m_adapter_name; }

    [[nodiscard]] RenderBackend backend() const override { return RenderBackend::Vulkan; }

    [[nodiscard]] BufferHandle create_buffer(const BufferDesc& desc) override;
    void update_buffer(BufferHandle handle, const void* data, usize size) override;
    void destroy_buffer(BufferHandle handle) override;
    [[nodiscard]] TextureHandle create_texture(const TextureDesc& desc) override;
    void destroy_texture(TextureHandle handle) override;
    [[nodiscard]] ShaderHandle create_shader(const ShaderDesc& desc) override;
    void destroy_shader(ShaderHandle handle) override;

    void begin_frame() override;
    void set_viewport(u32 width, u32 height) override;
    void clear(Vec4 color) override;
    void draw(const DrawCommand& command) override;
    void end_frame() override;

    // Read back buffer contents (host-visible memory). Returns bytes copied.
    usize read_buffer(BufferHandle handle, void* out, usize size);

private:
    struct VkBufferData {
        u64 buffer = 0; // VkBuffer
        u64 memory = 0; // VkDeviceMemory
        usize size = 0;
    };

    VkInstance_T* m_instance = nullptr;
    VkPhysicalDevice_T* m_physical = nullptr;
    VkDevice_T* m_device = nullptr;
    VkQueue_T* m_queue = nullptr;
    u32 m_queue_family = 0;
    std::string m_adapter_name;

    std::unordered_map<u32, VkBufferData> m_buffers;
    u32 m_next_id = 1;
};

} // namespace vyro
