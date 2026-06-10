// VyroEngine — Runtime entry point
// Boots the engine, exercises the foundation subsystems, and shuts down.
#include "vyro/core/Engine.hpp"
#include "vyro/core/Log.hpp"

#include <cstdio>

namespace {

struct TickEvent {
    int frame = 0;
};

} // namespace

int main(int /*argc*/, char** /*argv*/)
{
    vyro::Engine::print_banner();

    vyro::Engine engine;
    if (!engine.init()) {
        std::fprintf(stderr, "[VyroEngine] FATAL: engine initialization failed\n");
        return 1;
    }

    // Demonstrate the wired foundation: events, input action mapping, assets.
    engine.events().subscribe<TickEvent>(
        [](const TickEvent& e) { VYRO_INFO("Loop", "tick {}", e.frame); });

    engine.input().bind_action("Quit", vyro::KeyCode::Escape);

    constexpr int kFrames = 3;
    for (int frame = 1; frame <= kFrames; ++frame) {
        engine.input().new_frame();
        engine.events().publish(TickEvent{frame});
    }

    engine.shutdown();
    VYRO_INFO("Core", "Shutdown complete");
    return 0;
}
