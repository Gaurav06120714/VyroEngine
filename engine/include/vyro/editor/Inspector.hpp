// VyroEngine — Inspector property reflection
// Phase 8.3: a lightweight reflection registry. Components register named
// float properties with getter/setter accessors; the inspector panel (and
// serialization, scripting bindings) enumerate and edit them generically.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/ecs/Entity.hpp"
#include "vyro/ecs/Registry.hpp"

#include <functional>
#include <string>
#include <vector>

namespace vyro {

class Inspector
{
public:
    struct Property {
        std::string component;
        std::string name;
        std::function<bool(Registry&, Entity)> present; // component exists?
        std::function<f32(Registry&, Entity)> get;
        std::function<void(Registry&, Entity, f32)> set;
    };

    // Register a float field of component T.
    template<typename T>
    void register_property(std::string component, std::string name, f32 T::* member)
    {
        Property p;
        p.component = std::move(component);
        p.name = std::move(name);
        p.present = [](Registry& r, Entity e) { return r.has_component<T>(e); };
        p.get = [member](Registry& r, Entity e) {
            const T* c = r.get_component<T>(e);
            return c != nullptr ? c->*member : 0.0f;
        };
        p.set = [member](Registry& r, Entity e, f32 v) {
            T* c = r.get_component<T>(e);
            if (c != nullptr) {
                c->*member = v;
            }
        };
        m_properties.push_back(std::move(p));
    }

    // All properties present on `entity` (what the inspector panel shows).
    [[nodiscard]] std::vector<const Property*> properties_of(Registry& registry, Entity entity) const
    {
        std::vector<const Property*> out;
        for (const Property& p : m_properties) {
            if (p.present(registry, entity)) {
                out.push_back(&p);
            }
        }
        return out;
    }

    [[nodiscard]] usize property_count() const { return m_properties.size(); }

private:
    std::vector<Property> m_properties;
};

} // namespace vyro
