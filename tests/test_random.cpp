// VyroEngine — Weighted selection tests (V7.2)
#include "vyro/core/Random.hpp"

#include "test_harness.hpp"

#include <vector>

int main()
{
    using namespace vyro;
    test::Suite suite("random");

    // Three entries weighted 70/20/10.
    const std::vector<f32> w{70.0f, 20.0f, 10.0f};
    suite.check(weighted_index(w, 0.0f) == 0, "roll 0 hits the first bucket");
    suite.check(weighted_index(w, 0.5f) == 0, "mid-first-bucket stays in bucket 0");
    suite.check(weighted_index(w, 0.8f) == 1, "roll 0.8 lands in bucket 1");
    suite.check(weighted_index(w, 0.95f) == 2, "roll 0.95 lands in bucket 2");
    suite.check(weighted_index(w, 1.0f) == 2, "roll 1 clamps to the last bucket");

    // Zero-weight entries are skipped.
    {
        const std::vector<f32> z{0.0f, 1.0f, 0.0f};
        suite.check(weighted_index(z, 0.5f) == 1, "only the non-zero entry is picked");
    }

    // Degenerate tables are safe.
    {
        const std::vector<f32> empty{};
        suite.check(weighted_index(empty, 0.5f) == 0, "empty table returns 0");
        const std::vector<f32> zeros{0.0f, 0.0f};
        suite.check(weighted_index(zeros, 0.5f) == 0, "all-zero table returns 0");
    }

    return suite.summary();
}
