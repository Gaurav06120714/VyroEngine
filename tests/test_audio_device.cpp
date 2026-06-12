// VyroEngine — Real audio device tests (V3.3)
// Opens the actual output device and plays a synthesized voice to completion.
// Skips gracefully on machines with no audio output (CI).
#include "vyro/audio/AudioDevice.hpp"
#include "vyro/audio/SoundSynth.hpp"

#include "test_harness.hpp"

#include <chrono>
#include <thread>

int main()
{
    using namespace vyro;
    test::Suite suite("audio_device");

    AudioDevice device;
    if (!device.init()) {
        suite.check(true, "no audio output available; skipping");
        return suite.summary();
    }

    suite.check(device.is_open(), "device open");
    device.set_master_gain(0.05f); // quiet for the test run

    device.play(synth::hit(0.15f));
    suite.check(device.active_voices() == 1, "voice active after play");

    // The voice frees itself once consumed by the callback.
    bool finished = false;
    for (int i = 0; i < 100; ++i) {
        if (device.active_voices() == 0) {
            finished = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    suite.check(finished, "voice completed and was reclaimed");

    device.shutdown();
    suite.check(!device.is_open(), "device closed");
    return suite.summary();
}
