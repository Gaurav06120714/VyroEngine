// VyroEngine — Vulkan backend tests (V2.4 stage a)
// Initializes a real Vulkan device through MoltenVK and exercises the buffer
// resource layer with an upload/readback roundtrip.
#include "vyro/render/VulkanDevice.hpp"

#include "test_harness.hpp"

#include <cstdlib>
#include <filesystem>

namespace {

// Point the Vulkan loader at the Homebrew MoltenVK ICD when not already set.
void ensure_icd()
{
    if (std::getenv("VK_ICD_FILENAMES") != nullptr) {
        return;
    }
    const char* icd = "/opt/homebrew/opt/molten-vk/share/vulkan/icd.d/MoltenVK_icd.json";
    if (std::filesystem::exists(icd)) {
        setenv("VK_ICD_FILENAMES", icd, 1);
    }
}

} // namespace

int main()
{
    using namespace vyro;
    ensure_icd();
    test::Suite suite("vulkan");

    VulkanDevice device;
    if (!device.valid()) {
        // No Vulkan driver available (CI, old machine): report and pass
        // vacuously rather than fail unrelated environments.
        suite.check(true, "no Vulkan driver; skipping (device init unavailable)");
        return suite.summary();
    }

    // VulkanDevice_Init_IdentifiesAdapter
    suite.check(device.backend() == RenderBackend::Vulkan, "backend reports Vulkan");
    suite.check(!device.adapter_name().empty(), "adapter name discovered");

    // VulkanDevice_Buffer_UploadReadbackRoundtrip
    {
        const float payload[4] = {1.5f, -2.0f, 3.25f, 42.0f};
        BufferDesc desc;
        desc.type = BufferType::Vertex;
        desc.size = sizeof(payload);
        desc.data = payload;
        const BufferHandle buf = device.create_buffer(desc);
        suite.check(buf.valid(), "buffer created on GPU-visible memory");

        float back[4] = {};
        const usize n = device.read_buffer(buf, back, sizeof(back));
        suite.check(n == sizeof(back), "full readback size");
        suite.check(back[0] == 1.5f && back[3] == 42.0f, "payload survives roundtrip");

        const float updated[4] = {9.0f, 9.0f, 9.0f, 9.0f};
        device.update_buffer(buf, updated, sizeof(updated));
        device.read_buffer(buf, back, sizeof(back));
        suite.check(back[0] == 9.0f, "update_buffer rewrites contents");

        device.destroy_buffer(buf);
        suite.check(device.read_buffer(buf, back, sizeof(back)) == 0,
                    "destroyed buffer no longer readable");
    }

    return suite.summary();
}
