// VyroEngine — Entity manager
// Phase 2.1: allocates and recycles entity handles. Destroyed indices are
// reused with a bumped generation so old handles compare unequal.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/ecs/Entity.hpp"

#include <vector>

namespace vyro {

class EntityManager : NonCopyable
{
public:
    EntityManager() = default;

    // Create a live entity, reusing a freed index when available.
    [[nodiscard]] Entity create();

    // Destroy an entity. Its index is recycled and its generation bumped.
    void destroy(Entity entity);

    [[nodiscard]] bool is_alive(Entity entity) const;

    // Number of currently live entities.
    [[nodiscard]] usize alive_count() const { return m_alive_count; }

    // Highest index ever allocated (sizing hint for component storage).
    [[nodiscard]] usize capacity() const { return m_generations.size(); }

    // Snapshot of all currently live entities (serialization, tooling).
    [[nodiscard]] std::vector<Entity> alive_entities() const;

    void clear();

private:
    std::vector<u32> m_generations; // current generation per index
    std::vector<bool> m_alive;      // liveness per index
    std::vector<u32> m_free;        // recycled indices
    usize m_alive_count = 0;
};

} // namespace vyro
