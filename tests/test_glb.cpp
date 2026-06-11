// VyroEngine — GLB loader tests
// Loads the bundled character/weapon models (real Poly Pizza assets).
#include "vyro/assets/GlbLoader.hpp"

#include "test_harness.hpp"

#include <string>

namespace {

std::expected<vyro::GlbModel, vyro::GlbError> load_any(const char* rel)
{
    for (const std::string prefix : {"", "../", "../../"}) {
        auto r = vyro::load_glb(prefix + rel);
        if (r.has_value() || r.error() != vyro::GlbError::FileNotFound) {
            return r;
        }
    }
    return std::unexpected(vyro::GlbError::FileNotFound);
}

} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("glb");

    // Zombie: rigged character with an embedded texture.
    {
        const auto model = load_any("assets/models/characters/zombie_animated.glb");
        suite.check(model.has_value(), "zombie GLB loads");
        if (model.has_value()) {
            suite.check(model->mesh.vertices.size() > 100, "zombie has real geometry");
            suite.check(model->mesh.indices.size() % 3 == 0, "indices form triangles");
            suite.check(model->has_texture, "zombie has an embedded texture");
            suite.check(model->texture.width > 0 && model->texture.height > 0,
                        "texture decoded");
        }
    }

    // Rifle: static prop.
    {
        const auto model = load_any("assets/models/weapons/assault_rifle.glb");
        suite.check(model.has_value(), "rifle GLB loads");
        if (model.has_value()) {
            suite.check(model->mesh.vertices.size() > 10, "rifle has geometry");
        }
    }

    // Bad input handling.
    {
        suite.check(!load_glb("/no/such/file.glb").has_value(), "missing file errors");
    }

    return suite.summary();
}
