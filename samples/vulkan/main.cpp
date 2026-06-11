// VyroEngine — Vulkan on-screen demo (V2.4 stage b)
// Renders the project model in a real window through Vulkan (MoltenVK):
// surface + swapchain + depth buffer + SPIR-V pipeline + per-frame command
// recording with push-constant matrices. Single frame in flight, device-idle
// synchronization — bring-up first, optimization later (per the rulz, with a
// profiler to justify it).
#include "vyro/assets/Mesh.hpp"
#include "vyro/assets/ObjLoader.hpp"
#include "vyro/core/Engine.hpp"
#include "vyro/core/Log.hpp"
#include "vyro/math/Mat4.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

#define VK_CHECK(call)                                                        \
    do {                                                                      \
        const VkResult vk_result_ = (call);                                   \
        if (vk_result_ != VK_SUCCESS) {                                       \
            VYRO_ERROR("Vulkan", "{} failed ({})", #call,                      \
                       static_cast<int>(vk_result_));                          \
            std::exit(1);                                                     \
        }                                                                     \
    } while (false)

std::vector<char> read_file(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        return {};
    }
    const auto size = f.tellg();
    std::vector<char> data(static_cast<size_t>(size));
    f.seekg(0);
    f.read(data.data(), size);
    return data;
}

std::string find_asset(const char* rel)
{
    for (const std::string prefix : {"", "../"}) {
        const std::string p = prefix + rel;
        if (std::filesystem::exists(p)) {
            return p;
        }
    }
    return rel;
}

vyro::MeshData load_model()
{
    for (const char* p : {"assets/models/model.obj", "../assets/models/model.obj",
                          "assets/models/cube.obj", "../assets/models/cube.obj"}) {
        auto r = vyro::load_obj(p);
        if (r.has_value()) {
            VYRO_INFO("VkDemo", "loaded '{}'", p);
            return std::move(r.value());
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
    const vyro::Vec3 c{(lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f, (lo.z + hi.z) * 0.5f};
    const vyro::f32 extent = std::max({hi.x - lo.x, hi.y - lo.y, hi.z - lo.z});
    const vyro::f32 s = extent > 0.0f ? 2.0f / extent : 1.0f;
    return vyro::Mat4::scale({s, s, s}) * vyro::Mat4::translation(-c);
}

u_int32_t find_memory_type(VkPhysicalDevice physical, u_int32_t bits, VkMemoryPropertyFlags want)
{
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(physical, &props);
    for (u_int32_t i = 0; i < props.memoryTypeCount; ++i) {
        if ((bits & (1u << i)) != 0 && (props.memoryTypes[i].propertyFlags & want) == want) {
            return i;
        }
    }
    return 0;
}

struct PushConstants {
    vyro::Mat4 mvp;
    vyro::Mat4 model;
};

} // namespace

int main()
{
    vyro::Engine::print_banner();

    // MoltenVK ICD discovery for the Homebrew loader. Only point the loader at
    // a file that exists — a bad VK_ICD_FILENAMES hides every driver.
    if (std::getenv("VK_ICD_FILENAMES") == nullptr) {
        const char* icd = "/opt/homebrew/opt/molten-vk/etc/vulkan/icd.d/MoltenVK_icd.json";
        if (std::filesystem::exists(icd)) {
            setenv("VK_ICD_FILENAMES", icd, 1);
        }
    }

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1024, 720, "VyroEngine - Vulkan", nullptr, nullptr);
    if (window == nullptr || glfwVulkanSupported() == GLFW_FALSE) {
        VYRO_ERROR("VkDemo", "no Vulkan-capable window");
        return 1;
    }

    // ── Instance ─────────────────────────────────────────────────────
    u_int32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    std::vector<const char*> instance_exts(glfw_exts, glfw_exts + glfw_ext_count);
    instance_exts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "VyroEngine";
    app.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = static_cast<u_int32_t>(instance_exts.size());
    ici.ppEnabledExtensionNames = instance_exts.data();

    VkInstance instance = VK_NULL_HANDLE;
    VK_CHECK(vkCreateInstance(&ici, nullptr, &instance));

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));

    // ── Physical + logical device ────────────────────────────────────
    u_int32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    std::vector<VkPhysicalDevice> physicals(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, physicals.data());
    VkPhysicalDevice physical = physicals[0];

    VkPhysicalDeviceProperties pd_props;
    vkGetPhysicalDeviceProperties(physical, &pd_props);
    VYRO_INFO("VkDemo", "GPU: {}", pd_props.deviceName);

    u_int32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count, nullptr);
    std::vector<VkQueueFamilyProperties> families(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count, families.data());
    u_int32_t queue_family = 0;
    for (u_int32_t i = 0; i < family_count; ++i) {
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &present);
        if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && present == VK_TRUE) {
            queue_family = i;
            break;
        }
    }

    const float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = queue_family;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    const char* device_exts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"};
    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 2;
    dci.ppEnabledExtensionNames = device_exts;

    VkDevice device = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDevice(physical, &dci, nullptr, &device));
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queue_family, 0, &queue);

    // ── Swapchain ────────────────────────────────────────────────────
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps);
    const VkExtent2D extent = caps.currentExtent;

    u_int32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &format_count, formats.data());
    VkSurfaceFormatKHR fmt = formats[0];
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM) {
            fmt = f;
            break;
        }
    }

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = surface;
    sci.minImageCount = std::max(caps.minImageCount, 2u);
    sci.imageFormat = fmt.format;
    sci.imageColorSpace = fmt.colorSpace;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped = VK_TRUE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSwapchainKHR(device, &sci, nullptr, &swapchain));

    u_int32_t image_count = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    std::vector<VkImage> images(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data());

    std::vector<VkImageView> views(image_count);
    for (u_int32_t i = 0; i < image_count; ++i) {
        VkImageViewCreateInfo vci{};
        vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image = images[i];
        vci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vci.format = fmt.format;
        vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VK_CHECK(vkCreateImageView(device, &vci, nullptr, &views[i]));
    }

    // ── Depth buffer ─────────────────────────────────────────────────
    const VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
    VkImageCreateInfo dimg{};
    dimg.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    dimg.imageType = VK_IMAGE_TYPE_2D;
    dimg.format = depth_format;
    dimg.extent = {extent.width, extent.height, 1};
    dimg.mipLevels = 1;
    dimg.arrayLayers = 1;
    dimg.samples = VK_SAMPLE_COUNT_1_BIT;
    dimg.tiling = VK_IMAGE_TILING_OPTIMAL;
    dimg.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImage depth_image = VK_NULL_HANDLE;
    VK_CHECK(vkCreateImage(device, &dimg, nullptr, &depth_image));

    VkMemoryRequirements dreq;
    vkGetImageMemoryRequirements(device, depth_image, &dreq);
    VkMemoryAllocateInfo dalloc{};
    dalloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    dalloc.allocationSize = dreq.size;
    dalloc.memoryTypeIndex =
        find_memory_type(physical, dreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkDeviceMemory depth_memory = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateMemory(device, &dalloc, nullptr, &depth_memory));
    vkBindImageMemory(device, depth_image, depth_memory, 0);

    VkImageViewCreateInfo dview{};
    dview.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    dview.image = depth_image;
    dview.viewType = VK_IMAGE_VIEW_TYPE_2D;
    dview.format = depth_format;
    dview.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
    VkImageView depth_view = VK_NULL_HANDLE;
    VK_CHECK(vkCreateImageView(device, &dview, nullptr, &depth_view));

    // ── Render pass ──────────────────────────────────────────────────
    VkAttachmentDescription attachments[2]{};
    attachments[0].format = fmt.format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[1].format = depth_format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_ref{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pDepthStencilAttachment = &depth_ref;

    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 2;
    rpci.pAttachments = attachments;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VK_CHECK(vkCreateRenderPass(device, &rpci, nullptr, &render_pass));

    std::vector<VkFramebuffer> framebuffers(image_count);
    for (u_int32_t i = 0; i < image_count; ++i) {
        const VkImageView fb_views[2] = {views[i], depth_view};
        VkFramebufferCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = render_pass;
        fci.attachmentCount = 2;
        fci.pAttachments = fb_views;
        fci.width = extent.width;
        fci.height = extent.height;
        fci.layers = 1;
        VK_CHECK(vkCreateFramebuffer(device, &fci, nullptr, &framebuffers[i]));
    }

    // ── Pipeline (SPIR-V shaders + Vertex3D layout + push constants) ─
    auto make_module = [&](const std::string& path) {
        const std::vector<char> code = read_file(path);
        if (code.empty()) {
            VYRO_ERROR("VkDemo", "missing shader '{}' (build compiles them)", path);
            std::exit(1);
        }
        VkShaderModuleCreateInfo smci{};
        smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smci.codeSize = code.size();
        smci.pCode = reinterpret_cast<const u_int32_t*>(code.data());
        VkShaderModule module = VK_NULL_HANDLE;
        VK_CHECK(vkCreateShaderModule(device, &smci, nullptr, &module));
        return module;
    };
    const VkShaderModule vert = make_module(find_asset("assets/shaders/model.vert.spv"));
    const VkShaderModule frag = make_module(find_asset("assets/shaders/model.frag.spv"));

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{0, sizeof(vyro::Vertex3D), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrs[4]{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vyro::Vertex3D, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vyro::Vertex3D, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vyro::Vertex3D, uv)},
        {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vyro::Vertex3D, color)},
    };
    VkPipelineVertexInputStateCreateInfo vin{};
    vin.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vin.vertexBindingDescriptionCount = 1;
    vin.pVertexBindingDescriptions = &binding;
    vin.vertexAttributeDescriptionCount = 4;
    vin.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{0.0f, 0.0f, static_cast<float>(extent.width),
                        static_cast<float>(extent.height), 0.0f, 1.0f};
    VkRect2D scissor{{0, 0}, extent};
    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.pViewports = &viewport;
    vp.scissorCount = 1;
    vp.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                      | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blend_attachment;

    VkPushConstantRange push_range{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants)};
    VkPipelineLayoutCreateInfo plci{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &push_range;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreatePipelineLayout(device, &plci, nullptr, &layout));

    VkGraphicsPipelineCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount = 2;
    pci.pStages = stages;
    pci.pVertexInputState = &vin;
    pci.pInputAssemblyState = &ia;
    pci.pViewportState = &vp;
    pci.pRasterizationState = &rs;
    pci.pMultisampleState = &ms;
    pci.pDepthStencilState = &ds;
    pci.pColorBlendState = &blend;
    pci.layout = layout;
    pci.renderPass = render_pass;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline));
    vkDestroyShaderModule(device, vert, nullptr);
    vkDestroyShaderModule(device, frag, nullptr);

    // ── Geometry buffers (host-visible) ──────────────────────────────
    const vyro::MeshData mesh = load_model();
    auto make_buffer = [&](VkBufferUsageFlags usage, const void* data, size_t size) {
        VkBufferCreateInfo bci{};
        bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size = size;
        bci.usage = usage;
        VkBuffer buffer = VK_NULL_HANDLE;
        VK_CHECK(vkCreateBuffer(device, &bci, nullptr, &buffer));
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(device, buffer, &req);
        VkMemoryAllocateInfo mai{};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = req.size;
        mai.memoryTypeIndex = find_memory_type(
            physical, req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VK_CHECK(vkAllocateMemory(device, &mai, nullptr, &memory));
        vkBindBufferMemory(device, buffer, memory, 0);
        void* mapped = nullptr;
        vkMapMemory(device, memory, 0, size, 0, &mapped);
        std::memcpy(mapped, data, size);
        vkUnmapMemory(device, memory);
        return std::pair{buffer, memory};
    };
    const auto [vbo, vbo_mem] = make_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                            mesh.vertices.data(), mesh.vertex_bytes());
    const auto [ibo, ibo_mem] = make_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                            mesh.indices.data(), mesh.index_bytes());

    // ── Commands + sync ──────────────────────────────────────────────
    VkCommandPoolCreateInfo cpci{};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = queue_family;
    VkCommandPool pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateCommandPool(device, &cpci, nullptr, &pool));

    VkCommandBufferAllocateInfo cbai{};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(device, &cbai, &cmd));

    VkSemaphoreCreateInfo sem_ci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
    VkSemaphore image_ready = VK_NULL_HANDLE;
    VkSemaphore render_done = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSemaphore(device, &sem_ci, nullptr, &image_ready));
    VK_CHECK(vkCreateSemaphore(device, &sem_ci, nullptr, &render_done));

    // ── Camera ───────────────────────────────────────────────────────
    const float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
    vyro::Mat4 proj = vyro::Mat4::perspective(1.0472f, aspect, 0.1f, 100.0f);
    proj.at(1, 1) *= -1.0f; // Vulkan clip space Y points down
    const vyro::Mat4 view = vyro::Mat4::look_at({0.0f, 1.2f, 4.0f}, {0, 0, 0}, {0, 1, 0});
    const vyro::Mat4 fit = fit_transform(mesh);
    const auto start = std::chrono::steady_clock::now();

    VYRO_INFO("VkDemo", "Vulkan window open. Close it to quit.");

    // ── Frame loop (single frame in flight) ──────────────────────────
    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        glfwPollEvents();

        u_int32_t image_index = 0;
        const VkResult acquire = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                                                       image_ready, VK_NULL_HANDLE, &image_index);
        if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
            break; // resize/teardown handling is stage-c scope
        }

        const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();
        PushConstants pc;
        pc.model = vyro::Mat4::rotation({0, 1, 0}, t * 0.8f)
                   * vyro::Mat4::rotation({1, 0, 0}, 0.45f) * fit;
        pc.mvp = proj * view * pc.model;

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(cmd, &begin));

        VkClearValue clears[2];
        clears[0].color = {{0.06f, 0.07f, 0.10f, 1.0f}};
        clears[1].depthStencil = {1.0f, 0};
        VkRenderPassBeginInfo rpbi{};
        rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpbi.renderPass = render_pass;
        rpbi.framebuffer = framebuffers[image_index];
        rpbi.renderArea = {{0, 0}, extent};
        rpbi.clearValueCount = 2;
        rpbi.pClearValues = clears;
        vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        const VkDeviceSize zero = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vbo, &zero);
        vkCmdBindIndexBuffer(cmd, ibo, 0, VK_INDEX_TYPE_UINT32);
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
        vkCmdDrawIndexed(cmd, static_cast<u_int32_t>(mesh.indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmd);
        VK_CHECK(vkEndCommandBuffer(cmd));

        const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &image_ready;
        submit.pWaitDstStageMask = &wait_stage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &render_done;
        VK_CHECK(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));

        VkPresentInfoKHR present{};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &render_done;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &image_index;
        vkQueuePresentKHR(queue, &present);

        vkDeviceWaitIdle(device); // single frame in flight
    }

    // ── Teardown ─────────────────────────────────────────────────────
    vkDeviceWaitIdle(device);
    vkDestroySemaphore(device, image_ready, nullptr);
    vkDestroySemaphore(device, render_done, nullptr);
    vkDestroyCommandPool(device, pool, nullptr);
    vkDestroyBuffer(device, vbo, nullptr);
    vkFreeMemory(device, vbo_mem, nullptr);
    vkDestroyBuffer(device, ibo, nullptr);
    vkFreeMemory(device, ibo_mem, nullptr);
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, layout, nullptr);
    for (auto fb : framebuffers) {
        vkDestroyFramebuffer(device, fb, nullptr);
    }
    vkDestroyRenderPass(device, render_pass, nullptr);
    vkDestroyImageView(device, depth_view, nullptr);
    vkDestroyImage(device, depth_image, nullptr);
    vkFreeMemory(device, depth_memory, nullptr);
    for (auto v : views) {
        vkDestroyImageView(device, v, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
    VYRO_INFO("VkDemo", "Goodbye.");
    return 0;
}
