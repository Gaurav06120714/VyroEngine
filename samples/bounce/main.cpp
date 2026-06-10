// VyroEngine — Bouncing ball sample
// A visible demonstration of the engine: a real PhysicsWorld simulates a ball
// dropping onto a floor and bouncing, rendered as animated ASCII frames in the
// terminal. Exercises the physics, math, and logging subsystems together.
#include "vyro/core/Engine.hpp"
#include "vyro/core/Log.hpp"
#include "vyro/physics/PhysicsWorld.hpp"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

namespace {

constexpr int kWidth = 40;
constexpr int kHeight = 18;
constexpr vyro::f32 kWorldHeight = 12.0f; // world units mapped to kHeight rows

// Draw one frame: a side view with the floor at the bottom and the ball 'O'.
void render(const vyro::RigidBody& ball, int frame, vyro::f32 floor_top)
{
    const vyro::f32 cell = kWorldHeight / static_cast<vyro::f32>(kHeight);
    const int ball_col = kWidth / 2;
    const int ball_row = kHeight - 1 - static_cast<int>((ball.position.y - floor_top) / cell);

    std::printf("\033[H\033[2J"); // move cursor home + clear screen
    std::printf("  VyroEngine  -  physics demo   frame %02d   y=%.2f  vy=%.2f\n", frame,
                static_cast<double>(ball.position.y), static_cast<double>(ball.velocity.y));
    std::printf("  +");
    for (int c = 0; c < kWidth; ++c) {
        std::printf("-");
    }
    std::printf("+\n");

    for (int row = 0; row < kHeight; ++row) {
        std::printf("  |");
        for (int col = 0; col < kWidth; ++col) {
            if (row == ball_row && col == ball_col) {
                std::printf("O");
            } else {
                std::printf(" ");
            }
        }
        std::printf("|\n");
    }

    std::printf("  +");
    for (int c = 0; c < kWidth; ++c) {
        std::printf("=");
    }
    std::printf("+\n");
    std::fflush(stdout);
}

} // namespace

int main()
{
    vyro::Engine::print_banner();

    vyro::PhysicsWorld world;
    world.set_gravity(vyro::Vec3{0.0f, -9.81f, 0.0f});

    // Floor: a large static sphere whose top sits at y = 0 (locally flat).
    constexpr vyro::f32 floor_radius = 1000.0f;
    vyro::RigidBody floor;
    floor.position = vyro::Vec3{0.0f, -floor_radius, 0.0f};
    floor.radius = floor_radius;
    floor.set_mass(0.0f); // static
    floor.restitution = 0.85f;
    world.add_body(floor);

    // Ball: dropped from near the top of the view.
    vyro::RigidBody ball;
    ball.position = vyro::Vec3{0.0f, 10.0f, 0.0f};
    ball.radius = 0.4f;
    ball.set_mass(1.0f);
    ball.restitution = 0.85f;
    const vyro::BodyId ball_id = world.add_body(ball);

    VYRO_INFO("Demo", "Dropping ball from y={}", 10.0f);

    // Run the simulation and animate it. Several fixed steps per drawn frame.
    constexpr int kFrames = 90;
    constexpr int kStepsPerFrame = 2;
    for (int frame = 0; frame < kFrames; ++frame) {
        for (int s = 0; s < kStepsPerFrame; ++s) {
            world.step();
        }
        render(world.body(ball_id), frame, /*floor_top*/ 0.0f);
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 fps
    }

    VYRO_INFO("Demo", "Done after {} fixed steps", kFrames * kStepsPerFrame);
    return 0;
}
