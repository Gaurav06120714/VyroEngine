// VyroEngine — Minimal test harness
// A dependency-free assertion harness for the bootstrap phase. A full
// framework (GoogleTest, per rulz/TESTING_RULES) is integrated in the
// dedicated testing sub-phase; this keeps early tests offline and fast.
#pragma once

#include <cstdio>
#include <string_view>

namespace vyro::test {

class Suite
{
public:
    explicit Suite(std::string_view name) : m_name(name)
    {
        std::printf("[tests:%.*s]\n", static_cast<int>(m_name.size()), m_name.data());
    }

    void check(bool condition, std::string_view description)
    {
        if (condition) {
            std::printf("  ok:   %.*s\n", static_cast<int>(description.size()), description.data());
        } else {
            std::printf("  FAIL: %.*s\n", static_cast<int>(description.size()), description.data());
            ++m_failures;
        }
    }

    // Return 0 if all checks passed, 1 otherwise (suitable as a process code).
    [[nodiscard]] int summary() const
    {
        if (m_failures == 0) {
            std::printf("[tests:%.*s] ALL PASSED\n",
                        static_cast<int>(m_name.size()), m_name.data());
            return 0;
        }
        std::printf("[tests:%.*s] %d FAILURE(S)\n",
                    static_cast<int>(m_name.size()), m_name.data(), m_failures);
        return 1;
    }

private:
    std::string_view m_name;
    int              m_failures = 0;
};

} // namespace vyro::test
