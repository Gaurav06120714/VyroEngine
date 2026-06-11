// VyroEngine — Profiler tests
#include "vyro/core/Profiler.hpp"

#include "test_harness.hpp"

int main()
{
    using namespace vyro;
    test::Suite suite("profiler");
    Profiler::instance().reset();

    // Profiler_Record_AggregatesZoneStats (11.1)
    {
        Profiler::instance().record("Physics", 2.0);
        Profiler::instance().record("Physics", 4.0);
        Profiler::instance().record("Render", 8.0);

        const auto* physics = Profiler::instance().zone("Physics");
        suite.check(physics != nullptr, "zone exists");
        suite.check(physics->calls == 2, "two samples recorded");
        suite.check(physics->total_ms == 6.0, "total accumulates");
        suite.check(physics->peak_ms == 4.0, "peak tracked");
        suite.check(Profiler::instance().zone("Missing") == nullptr, "unknown zone is null");
    }

    // Profiler_Frames_AverageAndPeak (11.1)
    {
        Profiler::instance().end_frame(16.0);
        Profiler::instance().end_frame(20.0);
        suite.check(Profiler::instance().frame_count() == 2, "two frames");
        suite.check(Profiler::instance().average_frame_ms() == 18.0, "average frame time");
        suite.check(Profiler::instance().peak_frame_ms() == 20.0, "peak frame time");
    }

    // ScopedTimer_RecordsOnDestruction (11.1)
    {
        Profiler::instance().reset();
        {
            VYRO_PROFILE_SCOPE("Scoped");
        }
        const auto* zone = Profiler::instance().zone("Scoped");
        suite.check(zone != nullptr && zone->calls == 1, "scoped timer recorded once");
        suite.check(zone->total_ms >= 0.0, "non-negative duration");
    }

    return suite.summary();
}
