// VyroEngine — Run stats tests (V8.3)
#include "vyro/game/RunStats.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f32 a, vyro::f32 b, vyro::f32 eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("runstats");

    game::RunStats s;
    suite.check(approx(s.accuracy(), 0.0f), "no shots -> 0 accuracy");

    s.on_shots(10);
    s.on_hit();
    s.on_hit();
    s.on_hit();
    suite.check(s.shots == 10 && s.hits == 3, "shots and hits accumulate");
    suite.check(approx(s.accuracy(), 0.3f), "accuracy is hits/shots");

    s.on_kill(0);
    s.on_kill(2);
    s.on_kill(2);
    suite.check(s.kills == 3, "kills counted");
    suite.check(s.kills_by_type[2] == 2 && s.kills_by_type[0] == 1, "kills tracked per type");
    suite.check(s.kills_by_type[1] == 0, "untouched type stays zero");

    s.on_kill(999); // out of range — counts the kill, ignores the bucket
    suite.check(s.kills == 4, "out-of-range kill still counts");

    s.tick(0.5f);
    s.tick(0.25f);
    suite.check(approx(s.time, 0.75f), "time survived accumulates");

    s.reset();
    suite.check(s.shots == 0 && s.kills == 0 && approx(s.time, 0.0f), "reset clears the run");

    return suite.summary();
}
