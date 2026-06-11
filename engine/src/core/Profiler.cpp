// VyroEngine — Profiler implementation
#include "vyro/core/Profiler.hpp"

namespace vyro {

Profiler& Profiler::instance()
{
    static Profiler profiler;
    return profiler;
}

void Profiler::record(std::string_view zone, f64 milliseconds)
{
    ZoneStats& stats = m_zones[std::string(zone)];
    ++stats.calls;
    stats.total_ms += milliseconds;
    if (milliseconds > stats.peak_ms) {
        stats.peak_ms = milliseconds;
    }
}

void Profiler::end_frame(f64 frame_ms)
{
    ++m_frames;
    m_total_frame_ms += frame_ms;
    if (frame_ms > m_peak_frame_ms) {
        m_peak_frame_ms = frame_ms;
    }
}

const Profiler::ZoneStats* Profiler::zone(std::string_view name) const
{
    const auto it = m_zones.find(std::string(name));
    return it != m_zones.end() ? &it->second : nullptr;
}

f64 Profiler::average_frame_ms() const
{
    return m_frames > 0 ? m_total_frame_ms / static_cast<f64>(m_frames) : 0.0;
}

void Profiler::reset()
{
    m_zones.clear();
    m_total_frame_ms = 0.0;
    m_peak_frame_ms = 0.0;
    m_frames = 0;
}

} // namespace vyro
