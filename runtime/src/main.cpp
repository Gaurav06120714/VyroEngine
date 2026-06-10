// VyroEngine — Runtime entry point
// Boots the engine, runs a trivial loop, and shuts down cleanly.
#include "vyro/core/Engine.hpp"

#include <cstdio>

int main(int /*argc*/, char** /*argv*/) {
    vyro::Engine::printBanner();

    vyro::Engine engine;
    if (!engine.init()) {
        std::fprintf(stderr, "[VyroEngine] FATAL: engine initialization failed\n");
        return 1;
    }

    std::printf("[VyroEngine] Engine initialized: %s\n",
                engine.isInitialized() ? "true" : "false");

    // Placeholder main loop. Replaced by the real game loop in Phase 1.5+.
    constexpr int kFrames = 3;
    for (int frame = 0; frame < kFrames; ++frame) {
        std::printf("[VyroEngine] tick %d/%d\n", frame + 1, kFrames);
    }

    engine.shutdown();
    std::printf("[VyroEngine] Shutdown complete.\n");
    return 0;
}
