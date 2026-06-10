// VyroEngine — Engine facade
// Phase 1.1: minimal engine lifecycle (init / shutdown / banner).
// Subsystems (logging, events, input, memory, assets) are layered on in
// subsequent sub-phases.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro {

class Engine : NonCopyable {
public:
    Engine();
    ~Engine();

    // Initialize core subsystems. Returns false on failure.
    bool init();

    // Tear down subsystems in reverse order.
    void shutdown();

    // Print engine name/version banner to stdout.
    static void printBanner();

    [[nodiscard]] bool isInitialized() const { return m_initialized; }

private:
    bool m_initialized = false;
};

} // namespace vyro
