// VyroEngine — Logging tests
#include "vyro/core/Log.hpp"

#include "test_harness.hpp"

#include <string>

int main()
{
    vyro::test::Suite suite("log");

    // Logger_LevelName_ReturnsTag
    suite.check(vyro::Logger::level_name(vyro::LogLevel::Info) == "INFO",
                "level_name(Info) == INFO");
    suite.check(vyro::Logger::level_name(vyro::LogLevel::Error) == "ERROR",
                "level_name(Error) == ERROR");

    // Logger_Colors_ToggleRoundTrips
    vyro::Logger::set_colors_enabled(true);
    suite.check(vyro::Logger::colors_enabled(), "colors enabled after set(true)");
    vyro::Logger::set_colors_enabled(false);
    suite.check(!vyro::Logger::colors_enabled(), "colors disabled after set(false)");

    // Logger_Log_DoesNotThrow (smoke: formatting + output path)
    VYRO_INFO("Test", "hello {} {}", "world", 42);
    VYRO_WARN("Test", "warn value={}", 3.14);
    suite.check(true, "log macros executed without crashing");

    return suite.summary();
}
