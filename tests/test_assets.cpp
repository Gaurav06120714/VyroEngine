// VyroEngine — Asset management tests
#include "vyro/assets/AssetManager.hpp"

#include "test_harness.hpp"

#include <string>

namespace {

struct Mesh {
    std::string name;
    int triangles = 0;
};

} // namespace

int main()
{
    vyro::test::Suite suite("assets");

    // AssetManager_Load_ConstructsAndResolves
    {
        vyro::AssetManager mgr;
        auto mesh = mgr.load<Mesh>("cube.mesh", Mesh{"cube", 12});
        suite.check(mesh.is_valid(), "loaded handle is valid");
        suite.check(mesh->triangles == 12, "asset resolves to constructed data");
        suite.check(mgr.asset_count() == 1, "manager tracks one asset");
        suite.check(mgr.is_loaded("cube.mesh"), "is_loaded reports true");
    }

    // AssetManager_LoadSame_SharesInstance
    {
        vyro::AssetManager mgr;
        auto a = mgr.load<Mesh>("shared.mesh", Mesh{"a", 1});
        auto b = mgr.load<Mesh>("shared.mesh", Mesh{"b", 2});
        suite.check(mgr.asset_count() == 1, "same name shares one entry");
        suite.check(a.get() == b.get(), "handles resolve to same instance");
    }

    // AssetManager_RefCount_EvictsAtZero
    {
        vyro::AssetManager mgr;
        {
            auto h1 = mgr.load<Mesh>("temp.mesh", Mesh{"t", 3});
            {
                auto h2 = h1; // copy increments ref
                suite.check(mgr.asset_count() == 1, "copy keeps single entry");
            }
            suite.check(mgr.asset_count() == 1, "asset alive while one handle remains");
        }
        suite.check(mgr.asset_count() == 0, "asset evicted when last handle drops");
    }

    // AssetManager_Reload_UpdatesDataKeepsHandle
    {
        vyro::AssetManager mgr;
        auto mesh = mgr.load<Mesh>("hot.mesh", Mesh{"v1", 10});
        mgr.reload<Mesh>("hot.mesh", Mesh{"v2", 20});
        suite.check(mesh.is_valid(), "handle stays valid across reload");
        suite.check(mesh->triangles == 20, "handle observes reloaded data");
        suite.check(mesh->name == "v2", "reloaded name visible");
    }

    return suite.summary();
}
