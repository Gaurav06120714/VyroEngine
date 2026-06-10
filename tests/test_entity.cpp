// VyroEngine — Entity manager tests
#include "vyro/ecs/EntityManager.hpp"

#include "test_harness.hpp"

int main()
{
    vyro::test::Suite suite("entity");

    // EntityManager_Create_ProducesLiveEntity
    {
        vyro::EntityManager em;
        auto e = em.create();
        suite.check(em.is_alive(e), "created entity is alive");
        suite.check(em.alive_count() == 1, "alive count is 1");
        suite.check(!e.is_null(), "created entity is not null");
    }

    // EntityManager_Destroy_InvalidatesHandle
    {
        vyro::EntityManager em;
        auto e = em.create();
        em.destroy(e);
        suite.check(!em.is_alive(e), "destroyed entity not alive");
        suite.check(em.alive_count() == 0, "alive count back to 0");
    }

    // EntityManager_Recycle_BumpsGeneration
    {
        vyro::EntityManager em;
        auto a = em.create();
        em.destroy(a);
        auto b = em.create(); // reuses index
        suite.check(b.index == a.index, "index recycled");
        suite.check(b.generation == a.generation + 1, "generation bumped on reuse");
        suite.check(!em.is_alive(a), "old handle stale after recycle");
        suite.check(em.is_alive(b), "new handle alive");
    }

    // EntityManager_NullHandle_NeverAlive
    {
        vyro::EntityManager em;
        suite.check(!em.is_alive(vyro::kNullEntity), "null entity is not alive");
    }

    return suite.summary();
}
