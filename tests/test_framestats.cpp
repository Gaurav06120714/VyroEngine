// VyroEngine — Frame stats tests (V6.5)
// The EMA converges to a steady frame time, derives FPS, and flags the budget.
#include "vyro/core/FrameStats.hpp"

#include "test_harness.hpp"

#include <cmath>

namespace {
bool approx(vyro::f64 a, vyro::f64 b, vyro::f64 eps = 1e-2) { return std::fabs(a - b) < eps; }
} // namespace

int main()
{
    using namespace vyro;
    test::Suite suite("framestats");

    // First sample initializes the average exactly.
    {
        FrameStats fs(0.2);
        fs.add(16.0);
        suite.check(approx(fs.average_ms(), 16.0), "first sample sets the average");
    }

    // A constant feed converges to that value.
    {
        FrameStats fs(0.3);
        for (int i = 0; i < 200; ++i) {
            fs.add(20.0);
        }
        suite.check(approx(fs.average_ms(), 20.0), "constant feed converges");
        suite.check(approx(fs.fps(), 50.0, 0.5), "fps is 1000/ms");
    }

    // Budget check.
    {
        FrameStats fs(1.0); // react fully each sample
        fs.add(33.0);
        suite.check(fs.over_budget(16.6), "33ms is over a 60fps budget");
        fs.add(10.0);
        suite.check(!fs.over_budget(16.6), "10ms is under a 60fps budget");
    }

    // The EMA moves toward a new level but not instantly (smoothing).
    {
        FrameStats fs(0.1);
        fs.add(10.0);
        fs.add(30.0);
        suite.check(fs.average_ms() > 10.0 && fs.average_ms() < 30.0,
                    "EMA lags a sudden spike");
    }

    return suite.summary();
}
