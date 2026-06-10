// VyroEngine — Scene and scene manager tests
#include "vyro/scene/Scene.hpp"
#include "vyro/scene/SceneManager.hpp"

#include "test_harness.hpp"

namespace {

struct Counter {
    int ticks = 0;
};

class CounterSystem final : public vyro::ISystem
{
public:
    void on_update(vyro::Registry& reg, vyro::f32) override
    {
        reg.view<Counter>().for_each([](Counter& c) { c.ticks += 1; });
    }
};

} // namespace

int main()
{
    vyro::test::Suite suite("scene");

    // Scene_Update_RunsItsSystems
    {
        vyro::Scene scene("Level1");
        scene.systems().add_system<CounterSystem>();
        auto e = scene.create_entity();
        scene.registry().add_component<Counter>(e, Counter{0});

        scene.update(0.016f);
        scene.update(0.016f);
        suite.check(scene.registry().get_component<Counter>(e)->ticks == 2,
                    "scene update runs its systems");
        suite.check(scene.name() == "Level1", "scene reports its name");
    }

    // SceneManager_CreateAndFind_TracksScenes
    {
        vyro::SceneManager mgr;
        auto& menu = mgr.create_scene("Menu");
        mgr.create_scene("Game");
        suite.check(mgr.scene_count() == 2, "manager owns two scenes");
        suite.check(mgr.find("Menu") == &menu, "find resolves scene by name");
        suite.check(mgr.find("Missing") == nullptr, "find returns null for unknown");
    }

    // SceneManager_SetActive_ReplacesActiveSet
    {
        vyro::SceneManager mgr;
        auto& a = mgr.create_scene("A");
        auto& b = mgr.create_scene("B");
        mgr.set_active(a);
        suite.check(mgr.primary_active() == &a, "A is primary active");
        mgr.set_active(b);
        suite.check(mgr.active_count() == 1, "set_active replaces the active set");
        suite.check(mgr.primary_active() == &b, "B is now primary active");
    }

    // SceneManager_Additive_UpdatesAllActive
    {
        vyro::SceneManager mgr;
        auto& world = mgr.create_scene("World");
        auto& hud = mgr.create_scene("HUD");
        world.systems().add_system<CounterSystem>();
        hud.systems().add_system<CounterSystem>();
        auto we = world.create_entity();
        world.registry().add_component<Counter>(we, Counter{});
        auto he = hud.create_entity();
        hud.registry().add_component<Counter>(he, Counter{});

        mgr.set_active(world);
        mgr.add_active(hud);
        suite.check(mgr.active_count() == 2, "two scenes active additively");
        mgr.update(0.0f);
        suite.check(world.registry().get_component<Counter>(we)->ticks == 1, "world updated");
        suite.check(hud.registry().get_component<Counter>(he)->ticks == 1, "additive HUD updated");
    }

    // SceneManager_Unload_RemovesScene
    {
        vyro::SceneManager mgr;
        auto& temp = mgr.create_scene("Temp");
        mgr.set_active(temp);
        mgr.unload(temp);
        suite.check(mgr.scene_count() == 0, "scene destroyed on unload");
        suite.check(mgr.active_count() == 0, "unloaded scene removed from active set");
    }

    return suite.summary();
}
