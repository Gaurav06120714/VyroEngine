// VyroEngine — Bootstrap tests
#include "vyro/core/Engine.hpp"
#include "vyro/core/Version.hpp"

#include <cstdio>
#include <cstring>

static int g_failures = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::printf("  FAIL: %s (line %d)\n", #cond, __LINE__);      \
            ++g_failures;                                                \
        } else {                                                         \
            std::printf("  ok:   %s\n", #cond);                          \
        }                                                                \
    } while (0)

int main()
{
    std::printf("[tests] VyroEngine core tests\n");

    // Version sanity.
    CHECK(vyro::kVersionMajor == 1);
    CHECK(std::strcmp(vyro::kVersionString, "1.0.0") == 0);
    CHECK(std::strcmp(vyro::kEngineName, "VyroEngine") == 0);

    // Engine lifecycle.
    vyro::Engine engine;
    CHECK(!engine.is_initialized());
    CHECK(engine.init());
    CHECK(engine.is_initialized());
    engine.shutdown();
    CHECK(!engine.is_initialized());

    // Idempotent init/shutdown.
    CHECK(engine.init());
    CHECK(engine.init());  // second init is a no-op success
    engine.shutdown();
    engine.shutdown();     // second shutdown is safe

    if (g_failures == 0) {
        std::printf("[tests] ALL PASSED\n");
        return 0;
    }
    std::printf("[tests] %d FAILURE(S)\n", g_failures);
    return 1;
}
