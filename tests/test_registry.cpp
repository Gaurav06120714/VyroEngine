// VyroEngine — Registry / component tests
#include "vyro/ecs/Registry.hpp"

#include "test_harness.hpp"

namespace {

struct Position {
    float x = 0.0f;
    float y = 0.0f;
};

struct Velocity {
    float dx = 0.0f;
    float dy = 0.0f;
};

struct Tag {
    int value = 0;
};

} // namespace

int main()
{
    vyro::test::Suite suite("registry");

    // Registry_AddComponent_StoresAndResolves
    {
        vyro::Registry reg;
        auto e = reg.create_entity();
        reg.add_component<Position>(e, Position{1.0f, 2.0f});
        suite.check(reg.has_component<Position>(e), "entity has Position");
        suite.check(reg.get_component<Position>(e)->x == 1.0f, "Position resolves");
        suite.check(!reg.has_component<Velocity>(e), "missing component reports false");
    }

    // Registry_RemoveComponent_Detaches
    {
        vyro::Registry reg;
        auto e = reg.create_entity();
        reg.add_component<Tag>(e, Tag{7});
        reg.remove_component<Tag>(e);
        suite.check(!reg.has_component<Tag>(e), "component removed");
        suite.check(reg.get_component<Tag>(e) == nullptr, "get returns null after remove");
    }

    // Registry_DestroyEntity_RemovesAllComponents
    {
        vyro::Registry reg;
        auto e = reg.create_entity();
        reg.add_component<Position>(e, Position{});
        reg.add_component<Velocity>(e, Velocity{});
        reg.destroy_entity(e);
        suite.check(!reg.is_alive(e), "entity destroyed");
        suite.check(reg.entity_count() == 0, "no live entities");
    }

    // View_ForEach_VisitsOnlyMatchingEntities
    {
        vyro::Registry reg;
        // Two entities with both components, one with only Position.
        auto a = reg.create_entity();
        reg.add_component<Position>(a, Position{0, 0});
        reg.add_component<Velocity>(a, Velocity{1, 1});
        auto b = reg.create_entity();
        reg.add_component<Position>(b, Position{10, 10});
        reg.add_component<Velocity>(b, Velocity{2, 2});
        auto c = reg.create_entity();
        reg.add_component<Position>(c, Position{5, 5}); // no Velocity

        int visited = 0;
        reg.view<Position, Velocity>().for_each([&](Position& p, Velocity& v) {
            p.x += v.dx;
            p.y += v.dy;
            ++visited;
        });
        suite.check(visited == 2, "view visits only entities with both components");
        suite.check(reg.get_component<Position>(a)->x == 1.0f, "system mutated a.x");
        suite.check(reg.get_component<Position>(b)->x == 12.0f, "system mutated b.x");
        suite.check(reg.get_component<Position>(c)->x == 5.0f, "non-matching entity untouched");
    }

    // View_ForEachEntity_ProvidesHandle
    {
        vyro::Registry reg;
        auto e = reg.create_entity();
        reg.add_component<Tag>(e, Tag{42});
        vyro::Entity seen = vyro::kNullEntity;
        reg.view<Tag>().for_each_entity([&](vyro::Entity ent, Tag& t) {
            seen = ent;
            t.value += 1;
        });
        suite.check(seen == e, "for_each_entity yields the entity handle");
        suite.check(reg.get_component<Tag>(e)->value == 43, "component mutated via view");
    }

    return suite.summary();
}
