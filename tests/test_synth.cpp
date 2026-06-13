// VyroEngine — Sound synthesis tests (V3.3, headless)
#include "vyro/audio/SoundSynth.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("synth");

    // Gunshot: short, loud, decaying.
    {
        const auto s = synth::gunshot();
        suite.check(s.size() == static_cast<usize>(0.18f * synth::kSampleRate),
                    "gunshot has expected length");
        suite.check(synth::peak(s) > 0.3f, "gunshot is audible");
        suite.check(synth::peak(s) <= 1.5f, "gunshot does not blow out");
        // Energy decays: the last 10% is much quieter than the first 10%.
        const usize tenth = s.size() / 10;
        std::vector<f32> head(s.begin(), s.begin() + static_cast<long>(tenth));
        std::vector<f32> tail(s.end() - static_cast<long>(tenth), s.end());
        suite.check(synth::peak(head) > synth::peak(tail) * 4.0f, "gunshot decays");
    }

    // Groan: longer and lower-energy than the gunshot's attack.
    {
        const auto g = synth::groan();
        suite.check(g.size() > synth::gunshot().size(), "groan is longer than gunshot");
        suite.check(synth::peak(g) > 0.2f, "groan is audible");
    }

    // Hit: deterministic (same call, same samples).
    {
        const auto a = synth::hit();
        const auto b = synth::hit();
        suite.check(a == b, "synthesis is deterministic");
        suite.check(synth::peak(a) > 0.3f, "hit is audible");
    }

    // Music loop: correct length for the tempo and audible (V4.2).
    {
        const auto m = synth::music_loop(120.0f, 2);   // 2 bars at 120 bpm
        const usize expected = static_cast<usize>(2 * 4 * (60.0f / 120.0f) * synth::kSampleRate);
        suite.check(m.size() == expected, "music length matches tempo/bars");
        suite.check(synth::peak(m) > 0.1f, "music is audible");
        suite.check(synth::peak(m) <= 1.0f, "music does not clip");
    }

    return suite.summary();
}
