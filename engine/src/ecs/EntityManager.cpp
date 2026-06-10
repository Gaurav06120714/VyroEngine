// VyroEngine — Entity manager implementation
#include "vyro/ecs/EntityManager.hpp"

namespace vyro {

Entity EntityManager::create()
{
    u32 index;
    if (!m_free.empty()) {
        index = m_free.back();
        m_free.pop_back();
    } else {
        index = static_cast<u32>(m_generations.size());
        m_generations.push_back(0);
        m_alive.push_back(false);
    }
    m_alive[index] = true;
    ++m_alive_count;
    return Entity{index, m_generations[index]};
}

void EntityManager::destroy(Entity entity)
{
    if (!is_alive(entity)) {
        return;
    }
    m_alive[entity.index] = false;
    ++m_generations[entity.index]; // invalidate outstanding handles
    m_free.push_back(entity.index);
    --m_alive_count;
}

bool EntityManager::is_alive(Entity entity) const
{
    return !entity.is_null()
        && entity.index < m_generations.size()
        && m_alive[entity.index]
        && m_generations[entity.index] == entity.generation;
}

void EntityManager::clear()
{
    m_generations.clear();
    m_alive.clear();
    m_free.clear();
    m_alive_count = 0;
}

} // namespace vyro
