// VyroEngine — Engine facade implementation
#include "vyro/core/Engine.hpp"

#include "vyro/core/Log.hpp"
#include "vyro/core/Version.hpp"

#include <cstdio>

namespace vyro {

Engine::Engine() = default;

Engine::~Engine()
{
    if (m_initialized) {
        shutdown();
    }
}

bool Engine::init()
{
    if (m_initialized) {
        return true;
    }
    // Future sub-phases initialize subsystems here:
    //   Memory -> Logging -> Events -> Input -> Assets -> ECS -> Renderer.
    VYRO_INFO("Core", "Initializing {} v{}", kEngineName, kVersionString);
    m_initialized = true;
    VYRO_INFO("Core", "Engine initialized");
    return true;
}

void Engine::shutdown()
{
    if (!m_initialized) {
        return;
    }
    VYRO_INFO("Core", "Shutting down engine");
    m_initialized = false;
}

void Engine::print_banner()
{
    std::printf(
        "============================================\n"
        "  %s v%s\n"
        "  Transparency over magic.\n"
        "============================================\n",
        kEngineName, kVersionString);
}

} // namespace vyro
