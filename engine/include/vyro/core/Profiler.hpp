// VyroEngine — Profiler
// Phase 11.1: scoped CPU timing zones aggregated per name (call count, total
// and peak time), plus frame statistics. Optimization is driven by these
// measurements, never speculation (rulz/PERFORMANCE_RULES).
#pragma once

#include "vyro/core/Types.hpp"

#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vyro {

class Profiler
{
public:
    struct ZoneStats {
        u64 calls = 0;
        f64 total_ms = 0.0;
        f64 peak_ms = 0.0;
    };

    static Profiler& instance();

    // Record a completed zone sample.
    void record(std::string_view zone, f64 milliseconds);

    // Frame bookkeeping: call once per frame.
    void end_frame(f64 frame_ms);

    [[nodiscard]] const ZoneStats* zone(std::string_view name) const;
    [[nodiscard]] f64 average_frame_ms() const;
    [[nodiscard]] f64 peak_frame_ms() const { return m_peak_frame_ms; }
    [[nodiscard]] u64 frame_count() const { return m_frames; }

    void reset();

private:
    std::unordered_map<std::string, ZoneStats> m_zones;
    f64 m_total_frame_ms = 0.0;
    f64 m_peak_frame_ms = 0.0;
    u64 m_frames = 0;
};

// RAII timing zone: measures from construction to destruction.
class ScopedTimer
{
public:
    explicit ScopedTimer(std::string_view zone)
        : m_zone(zone), m_start(std::chrono::steady_clock::now())
    {
    }

    ~ScopedTimer()
    {
        const auto end = std::chrono::steady_clock::now();
        const f64 ms = std::chrono::duration<f64, std::milli>(end - m_start).count();
        Profiler::instance().record(m_zone, ms);
    }

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    std::string m_zone;
    std::chrono::steady_clock::time_point m_start;
};

} // namespace vyro

#define VYRO_PROFILE_SCOPE(name) ::vyro::ScopedTimer vyro_profile_scope_##__LINE__(name)
