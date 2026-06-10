// VyroEngine — Logging System
// Phase 1.3: leveled, category-tagged, thread-safe logging with compile-time
// level stripping. Verbose levels incur zero cost in shipping builds.
#pragma once

#include "vyro/core/Types.hpp"

#include <format>
#include <string_view>
#include <utility>

namespace vyro {

enum class LogLevel : u8 {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    Fatal = 5,
    Off   = 6,
};

// Compile-time minimum level. Anything below this is stripped at compile time.
#if !defined(VYRO_ACTIVE_LOG_LEVEL)
    #if defined(NDEBUG)
        #define VYRO_ACTIVE_LOG_LEVEL ::vyro::LogLevel::Info
    #else
        #define VYRO_ACTIVE_LOG_LEVEL ::vyro::LogLevel::Trace
    #endif
#endif

class Logger
{
public:
    // Emit a fully formatted message. Thread-safe.
    static void log(LogLevel level, std::string_view category, std::string_view message);

    // Toggle ANSI color output (off by default for plain, redirectable logs).
    static void set_colors_enabled(bool enabled);
    [[nodiscard]] static bool colors_enabled();

    // Human-readable 4-char tag for a level (e.g. "INFO").
    [[nodiscard]] static std::string_view level_name(LogLevel level);
};

} // namespace vyro

// ─── Logging macros ──────────────────────────────────────────────────────
// Usage: VYRO_INFO("Core", "Engine started v{}", version);
// Levels below VYRO_ACTIVE_LOG_LEVEL compile to a no-op.
#define VYRO_LOG_IMPL(level, category, ...)                                     \
    do {                                                                        \
        if constexpr ((level) >= VYRO_ACTIVE_LOG_LEVEL) {                       \
            ::vyro::Logger::log((level), (category),                            \
                                ::std::format(__VA_ARGS__));                     \
        }                                                                       \
    } while (false)

#define VYRO_TRACE(category, ...) VYRO_LOG_IMPL(::vyro::LogLevel::Trace, category, __VA_ARGS__)
#define VYRO_DEBUG(category, ...) VYRO_LOG_IMPL(::vyro::LogLevel::Debug, category, __VA_ARGS__)
#define VYRO_INFO(category, ...)  VYRO_LOG_IMPL(::vyro::LogLevel::Info,  category, __VA_ARGS__)
#define VYRO_WARN(category, ...)  VYRO_LOG_IMPL(::vyro::LogLevel::Warn,  category, __VA_ARGS__)
#define VYRO_ERROR(category, ...) VYRO_LOG_IMPL(::vyro::LogLevel::Error, category, __VA_ARGS__)
#define VYRO_FATAL(category, ...) VYRO_LOG_IMPL(::vyro::LogLevel::Fatal, category, __VA_ARGS__)
