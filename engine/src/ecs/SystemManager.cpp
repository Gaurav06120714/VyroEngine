// VyroEngine — System manager implementation
#include "vyro/ecs/SystemManager.hpp"

#include "vyro/ecs/Registry.hpp"

namespace vyro {

void SystemManager::update(f32 dt)
{
    for (auto& system : m_systems) {
        system->on_update(m_registry, dt);
    }
}

void SystemManager::clear()
{
    // Detach in reverse registration order so dependents tear down first.
    for (auto it = m_systems.rbegin(); it != m_systems.rend(); ++it) {
        (*it)->on_detach(m_registry);
    }
    m_systems.clear();
}

} // namespace vyro
