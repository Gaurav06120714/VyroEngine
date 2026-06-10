// VyroEngine — Engine facade implementation
#include "vyro/core/Engine.hpp"
#include "vyro/core/Version.hpp"

#include <cstdio>

namespace vyro {

Engine::Engine() = default;

Engine::~Engine() {
    if (m_initialized) {
        shutdown();
    }
}

bool Engine::init() {
    if (m_initialized) {
        return true;
    }
    // Future sub-phases initialize subsystems here:
    //   Memory -> Logging -> Events -> Input -> Assets -> ECS -> Renderer.
    m_initialized = true;
    return true;
}

void Engine::shutdown() {
    if (!m_initialized) {
        return;
    }
    m_initialized = false;
}

void Engine::printBanner() {
    std::printf(
        "============================================\n"
        "  %s v%s\n"
        "  Transparency over magic.\n"
        "============================================\n",
        kEngineName, kVersionString);
}

} // namespace vyro
