// VyroEngine — Engine facade
// Phase 1: owns and wires the foundation subsystems (events, input, assets)
// behind a single lifecycle. Higher-level systems (ECS, renderer) are layered
// on in later phases.
#pragma once

#include "vyro/assets/AssetManager.hpp"
#include "vyro/core/EventBus.hpp"
#include "vyro/core/Types.hpp"
#include "vyro/platform/Input.hpp"

namespace vyro {

class Engine : NonCopyable
{
public:
    Engine();
    ~Engine();

    // Initialize core subsystems. Returns false on failure.
    bool init();

    // Tear down subsystems in reverse order.
    void shutdown();

    // Print engine name/version banner to stdout.
    static void print_banner();

    [[nodiscard]] bool is_initialized() const { return m_initialized; }

    // ── Subsystem access ─────────────────────────────────────────────
    [[nodiscard]] EventBus& events() { return m_events; }
    [[nodiscard]] Input& input() { return m_input; }
    [[nodiscard]] AssetManager& assets() { return m_assets; }

private:
    bool m_initialized = false;

    EventBus m_events;
    Input m_input;
    AssetManager m_assets;
};

} // namespace vyro
