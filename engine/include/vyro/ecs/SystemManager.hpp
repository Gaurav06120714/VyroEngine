// VyroEngine — System manager
// Phase 2.3: owns the ordered set of systems and drives their per-frame
// update against a registry. Systems run in registration order.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/ecs/System.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace vyro {

class Registry;

class SystemManager : NonCopyable
{
public:
    explicit SystemManager(Registry& registry) : m_registry(registry) {}
    ~SystemManager() { clear(); }

    // Construct, attach, and register a system of type T. Returns a reference
    // to the owned instance.
    template<typename T, typename... Args>
    T& add_system(Args&&... args)
    {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;
        ref.on_attach(m_registry);
        m_systems.push_back(std::move(system));
        return ref;
    }

    // Update every system in registration order.
    void update(f32 dt);

    // Detach and destroy all systems (reverse order).
    void clear();

    [[nodiscard]] usize count() const { return m_systems.size(); }

private:
    Registry& m_registry;
    std::vector<std::unique_ptr<ISystem>> m_systems;
};

} // namespace vyro
