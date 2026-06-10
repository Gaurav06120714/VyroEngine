// VyroEngine — Shader library tests
#include "vyro/render/NullDevice.hpp"
#include "vyro/render/Shader.hpp"

#include "test_harness.hpp"

int main()
{
    vyro::test::Suite suite("shader");

    // ShaderLibrary_Load_CreatesValidShader
    {
        vyro::NullDevice dev;
        vyro::ShaderLibrary lib(dev);
        auto& s = lib.load("sprite", {"vs", "fs"});
        suite.check(s.valid(), "loaded shader is valid");
        suite.check(s.name() == "sprite", "shader keeps its name");
        suite.check(lib.exists("sprite"), "library reports shader exists");
        suite.check(dev.stats().shaders_created == 1, "device created one shader");
    }

    // ShaderLibrary_LoadSame_ReturnsCachedWithoutRecreating
    {
        vyro::NullDevice dev;
        vyro::ShaderLibrary lib(dev);
        auto& a = lib.load("pbr", {"v", "f"});
        auto& b = lib.load("pbr", {"v", "f"});
        suite.check(&a == &b, "second load returns cached instance");
        suite.check(dev.stats().shaders_created == 1, "no second device shader created");
        suite.check(lib.count() == 1, "library holds one shader");
    }

    // ShaderLibrary_Reload_ReplacesHandle
    {
        vyro::NullDevice dev;
        vyro::ShaderLibrary lib(dev);
        auto& s = lib.load("hot", {"v1", "f1"});
        const vyro::ShaderHandle before = s.handle();
        lib.reload("hot", {"v2", "f2"});
        suite.check(lib.get("hot")->valid(), "shader still valid after reload");
        suite.check(!(lib.get("hot")->handle() == before), "reload replaced the handle");
        suite.check(dev.stats().shaders_created == 2, "reload created a new device shader");
    }

    // ShaderLibrary_Get_Missing_ReturnsNull
    {
        vyro::NullDevice dev;
        vyro::ShaderLibrary lib(dev);
        suite.check(lib.get("nope") == nullptr, "get returns null for unknown shader");
    }

    return suite.summary();
}
