// VyroEngine — System manager tests
#include "vyro/ecs/Registry.hpp"
#include "vyro/ecs/SystemManager.hpp"

#include "test_harness.hpp"

#include <vector>

namespace {

struct Position {
    float x = 0.0f;
};

struct Velocity {
    float dx = 0.0f;
};

// Integrates position from velocity each frame.
class MovementSystem final : public vyro::ISystem
{
public:
    void on_attach(vyro::Registry&) override { attached = true; }
    void on_detach(vyro::Registry&) override { detached = true; }
    std::string_view name() const override { return "Movement"; }

    void on_update(vyro::Registry& reg, vyro::f32 dt) override
    {
        reg.view<Position, Velocity>().for_each(
            [dt](Position& p, Velocity& v) { p.x += v.dx * dt; });
    }

    bool attached = false;
    bool detached = false;
};

// Records the order in which systems run within a frame.
class OrderSystem final : public vyro::ISystem
{
public:
    OrderSystem(int id, std::vector<int>& log) : m_id(id), m_log(&log) {}
    void on_update(vyro::Registry&, vyro::f32) override { m_log->push_back(m_id); }

private:
    int m_id;
    std::vector<int>* m_log;
};

// Writes to an external flag on detach so the test can observe it after the
// system itself has been destroyed by clear().
class DetachTracker final : public vyro::ISystem
{
public:
    explicit DetachTracker(bool& flag) : m_flag(&flag) {}
    void on_update(vyro::Registry&, vyro::f32) override {}
    void on_detach(vyro::Registry&) override { *m_flag = true; }

private:
    bool* m_flag;
};

} // namespace

int main()
{
    vyro::test::Suite suite("systems");

    // SystemManager_AddSystem_CallsOnAttach
    {
        vyro::Registry reg;
        vyro::SystemManager systems(reg);
        auto& movement = systems.add_system<MovementSystem>();
        suite.check(movement.attached, "on_attach called when system added");
        suite.check(systems.count() == 1, "manager tracks one system");
    }

    // SystemManager_Update_RunsSystemBehavior
    {
        vyro::Registry reg;
        vyro::SystemManager systems(reg);
        systems.add_system<MovementSystem>();

        auto e = reg.create_entity();
        reg.add_component<Position>(e, Position{0.0f});
        reg.add_component<Velocity>(e, Velocity{10.0f});

        systems.update(0.5f);
        suite.check(reg.get_component<Position>(e)->x == 5.0f, "movement integrated x by v*dt");
        systems.update(0.5f);
        suite.check(reg.get_component<Position>(e)->x == 10.0f, "movement accumulates across frames");
    }

    // SystemManager_Update_RunsInRegistrationOrder
    {
        vyro::Registry reg;
        vyro::SystemManager systems(reg);
        std::vector<int> log;
        systems.add_system<OrderSystem>(1, log);
        systems.add_system<OrderSystem>(2, log);
        systems.add_system<OrderSystem>(3, log);
        systems.update(0.0f);
        suite.check(log.size() == 3 && log[0] == 1 && log[1] == 2 && log[2] == 3,
                    "systems run in registration order");
    }

    // SystemManager_Clear_CallsOnDetach
    {
        vyro::Registry reg;
        vyro::SystemManager systems(reg);
        bool detached = false;
        systems.add_system<DetachTracker>(detached);
        systems.clear();
        suite.check(detached, "on_detach called on clear");
        suite.check(systems.count() == 0, "no systems after clear");
    }

    return suite.summary();
}
