// VyroEngine — Logging System implementation
#include "vyro/core/Log.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>

namespace vyro {

namespace {

std::mutex g_log_mutex;
std::atomic<bool> g_colors{false};

// ANSI color per level (used only when colors are enabled).
std::string_view color_for(LogLevel level)
{
    switch (level) {
        case LogLevel::Trace: return "\033[90m"; // bright black
        case LogLevel::Debug: return "\033[36m"; // cyan
        case LogLevel::Info:  return "\033[32m"; // green
        case LogLevel::Warn:  return "\033[33m"; // yellow
        case LogLevel::Error: return "\033[31m"; // red
        case LogLevel::Fatal: return "\033[1;31m"; // bold red
        default: return "";
    }
}

// HH:MM:SS.mmm wall-clock timestamp.
std::string timestamp()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf{};
#if defined(VYRO_PLATFORM_WINDOWS)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    return std::format("{:02}:{:02}:{:02}.{:03}", tm_buf.tm_hour, tm_buf.tm_min,
                       tm_buf.tm_sec, static_cast<int>(ms.count()));
}

} // namespace

void Logger::log(LogLevel level, std::string_view category, std::string_view message)
{
    if (level == LogLevel::Off) {
        return;
    }

    // Warnings and above go to stderr; everything else to stdout.
    std::FILE* stream = (level >= LogLevel::Warn) ? stderr : stdout;
    const bool use_color = g_colors.load(std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(g_log_mutex);
    if (use_color) {
        std::fprintf(stream, "%s[%s] [%-5s] [%.*s] %.*s\033[0m\n",
                     color_for(level).data(), timestamp().c_str(),
                     level_name(level).data(),
                     static_cast<int>(category.size()), category.data(),
                     static_cast<int>(message.size()), message.data());
    } else {
        std::fprintf(stream, "[%s] [%-5s] [%.*s] %.*s\n",
                     timestamp().c_str(), level_name(level).data(),
                     static_cast<int>(category.size()), category.data(),
                     static_cast<int>(message.size()), message.data());
    }
    if (level >= LogLevel::Warn) {
        std::fflush(stream);
    }
}

void Logger::set_colors_enabled(bool enabled)
{
    g_colors.store(enabled, std::memory_order_relaxed);
}

bool Logger::colors_enabled()
{
    return g_colors.load(std::memory_order_relaxed);
}

std::string_view Logger::level_name(LogLevel level)
{
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        case LogLevel::Off:   return "OFF";
    }
    return "?????";
}

} // namespace vyro
