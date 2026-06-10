// VyroEngine — System interface
// Phase 2.3: a system encapsulates behavior over component data. The system
// manager drives on_update once per frame; on_attach/on_detach bracket the
// system's lifetime for resource setup and teardown.
#pragma once

#include "vyro/core/Types.hpp"

#include <string_view>

namespace vyro {

class Registry;

class ISystem
{
public:
    virtual ~ISystem() = default;

    // Called once when the system is added to the manager.
    virtual void on_attach(Registry& /*registry*/) {}

    // Called once per frame with the delta time in seconds.
    virtual void on_update(Registry& registry, f32 dt) = 0;

    // Called once when the system is removed or the manager is cleared.
    virtual void on_detach(Registry& /*registry*/) {}

    [[nodiscard]] virtual std::string_view name() const { return "System"; }
};

} // namespace vyro
