// VyroEngine — Vulkan render device implementation (via MoltenVK on macOS)
#include "vyro/render/VulkanDevice.hpp"

#include "vyro/core/Log.hpp"

#include <vulkan/vulkan.h>

#include <cstring>
#include <vector>

namespace vyro {

namespace {

// Find a host-visible, host-coherent memory type for staging-style buffers.
u32 find_host_memory_type(VkPhysicalDevice physical, u32 type_bits)
{
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(physical, &props);
    const VkMemoryPropertyFlags want =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (u32 i = 0; i < props.memoryTypeCount; ++i) {
        if ((type_bits & (1u << i)) != 0
            && (props.memoryTypes[i].propertyFlags & want) == want) {
            return i;
        }
    }
    return 0;
}

} // namespace

VulkanDevice::VulkanDevice()
{
    // ── Instance (portability enumeration required for MoltenVK) ─────
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "VyroEngine";
    app.apiVersion = VK_API_VERSION_1_1;

    const char* extensions[] = {
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    };

    VkInstanceCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = 1;
    ici.ppEnabledExtensionNames = extensions;

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&ici, nullptr, &instance) != VK_SUCCESS) {
        VYRO_ERROR("Vulkan", "vkCreateInstance failed");
        return;
    }
    m_instance = instance;

    // ── Physical device ──────────────────────────────────────────────
    u32 count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0) {
        VYRO_ERROR("Vulkan", "no physical devices");
        return;
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());
    m_physical = devices[0];

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physical, &props);
    m_adapter_name = props.deviceName;

    // ── Queue family (graphics) ──────────────────────────────────────
    u32 family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical, &family_count, families.data());
    for (u32 i = 0; i < family_count; ++i) {
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            m_queue_family = i;
            break;
        }
    }

    // ── Logical device + queue ───────────────────────────────────────
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = m_queue_family;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    // MoltenVK exposes VK_KHR_portability_subset, which must be enabled.
    const char* device_extensions[] = {"VK_KHR_portability_subset"};

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = device_extensions;

    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(m_physical, &dci, nullptr, &device) != VK_SUCCESS) {
        VYRO_ERROR("Vulkan", "vkCreateDevice failed");
        return;
    }
    m_device = device;

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, m_queue_family, 0, &queue);
    m_queue = queue;

    VYRO_INFO("Vulkan", "device initialized: {}", m_adapter_name);
}

VulkanDevice::~VulkanDevice()
{
    if (m_device != nullptr) {
        for (auto& [id, buf] : m_buffers) {
            vkDestroyBuffer(m_device, reinterpret_cast<VkBuffer>(buf.buffer), nullptr);
            vkFreeMemory(m_device, reinterpret_cast<VkDeviceMemory>(buf.memory), nullptr);
        }
        vkDestroyDevice(m_device, nullptr);
    }
    if (m_instance != nullptr) {
        vkDestroyInstance(m_instance, nullptr);
    }
}

BufferHandle VulkanDevice::create_buffer(const BufferDesc& desc)
{
    if (m_device == nullptr || desc.size == 0) {
        return BufferHandle{};
    }

    VkBufferCreateInfo bci{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = desc.size;
    bci.usage = (desc.type == BufferType::Index) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                                                 : VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(m_device, &bci, nullptr, &buffer) != VK_SUCCESS) {
        return BufferHandle{};
    }

    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(m_device, buffer, &reqs);

    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = reqs.size;
    mai.memoryTypeIndex = find_host_memory_type(m_physical, reqs.memoryTypeBits);

    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (vkAllocateMemory(m_device, &mai, nullptr, &memory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, buffer, nullptr);
        return BufferHandle{};
    }
    vkBindBufferMemory(m_device, buffer, memory, 0);

    const u32 id = m_next_id++;
    m_buffers[id] = VkBufferData{reinterpret_cast<u64>(buffer),
                                 reinterpret_cast<u64>(memory), desc.size};

    if (desc.data != nullptr) {
        update_buffer(BufferHandle{id}, desc.data, desc.size);
    }
    return BufferHandle{id};
}

void VulkanDevice::update_buffer(BufferHandle handle, const void* data, usize size)
{
    const auto it = m_buffers.find(handle.id);
    if (it == m_buffers.end() || data == nullptr) {
        return;
    }
    void* mapped = nullptr;
    const auto memory = reinterpret_cast<VkDeviceMemory>(it->second.memory);
    const usize n = size < it->second.size ? size : it->second.size;
    if (vkMapMemory(m_device, memory, 0, n, 0, &mapped) == VK_SUCCESS) {
        std::memcpy(mapped, data, n);
        vkUnmapMemory(m_device, memory);
    }
}

usize VulkanDevice::read_buffer(BufferHandle handle, void* out, usize size)
{
    const auto it = m_buffers.find(handle.id);
    if (it == m_buffers.end() || out == nullptr) {
        return 0;
    }
    void* mapped = nullptr;
    const auto memory = reinterpret_cast<VkDeviceMemory>(it->second.memory);
    const usize n = size < it->second.size ? size : it->second.size;
    if (vkMapMemory(m_device, memory, 0, n, 0, &mapped) != VK_SUCCESS) {
        return 0;
    }
    std::memcpy(out, mapped, n);
    vkUnmapMemory(m_device, memory);
    return n;
}

void VulkanDevice::destroy_buffer(BufferHandle handle)
{
    const auto it = m_buffers.find(handle.id);
    if (it == m_buffers.end()) {
        return;
    }
    vkDestroyBuffer(m_device, reinterpret_cast<VkBuffer>(it->second.buffer), nullptr);
    vkFreeMemory(m_device, reinterpret_cast<VkDeviceMemory>(it->second.memory), nullptr);
    m_buffers.erase(it);
}

// Stage b (swapchain presentation, pipelines, command recording) fills these
// in; stage a is the resource layer.
TextureHandle VulkanDevice::create_texture(const TextureDesc& /*desc*/)
{
    return TextureHandle{m_next_id++};
}
void VulkanDevice::destroy_texture(TextureHandle /*handle*/) {}
ShaderHandle VulkanDevice::create_shader(const ShaderDesc& /*desc*/)
{
    return ShaderHandle{m_next_id++};
}
void VulkanDevice::destroy_shader(ShaderHandle /*handle*/) {}
void VulkanDevice::begin_frame() {}
void VulkanDevice::set_viewport(u32 /*width*/, u32 /*height*/) {}
void VulkanDevice::clear(Vec4 /*color*/) {}
void VulkanDevice::draw(const DrawCommand& /*command*/) {}
void VulkanDevice::end_frame() {}

} // namespace vyro
