// VyroEngine — Save data tests (V8.1)
// Save blobs round-trip through the string form, parsing is tolerant, and
// values are clamped.
#include "vyro/game/SaveData.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-3f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("savedata");

    // Round-trip.
    {
        game::SaveData s;
        s.high_score = 1234;
        s.best_wave = 5;
        s.master_volume = 0.6f;
        s.difficulty = 2;
        const game::SaveData back = game::parse(game::serialize(s));
        suite.check(back.high_score == 1234 && back.best_wave == 5 && back.difficulty == 2,
                    "ints survive round-trip");
        suite.check(approx(back.master_volume, 0.6f), "volume survives round-trip");
    }

    // Tolerant parse: unknown keys ignored, missing keys keep defaults.
    {
        const game::SaveData s = game::parse("high_score=99\ngarbage line\nfoo=bar\n");
        suite.check(s.high_score == 99, "known key parsed");
        suite.check(s.best_wave == 0 && approx(s.master_volume, 1.0f),
                    "missing keys keep defaults");
    }

    // Clamping.
    {
        const game::SaveData s = game::parse("master_volume=5.0\ndifficulty=9\nhigh_score=-3\n");
        suite.check(approx(s.master_volume, 1.0f), "volume clamps to 1");
        suite.check(s.difficulty == 2, "difficulty clamps to hard");
        suite.check(s.high_score == 0, "negative score clamps to 0");
    }

    // Empty input is all defaults.
    {
        const game::SaveData s = game::parse("");
        suite.check(s.high_score == 0 && s.difficulty == 1, "empty parses to defaults");
    }

    return suite.summary();
}
