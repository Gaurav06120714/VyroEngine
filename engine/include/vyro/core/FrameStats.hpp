// VyroEngine — Frame timing stats (V6.5)
// A small exponential-moving-average tracker for per-frame time, plus FPS and a
// budget check, used to drive the on-screen profiler readout/graph. Headless
// and tested; pairs with the per-zone `Profiler`.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro {

class FrameStats
{
public:
    // `alpha` is the EMA smoothing factor in (0,1]; higher reacts faster.
    explicit FrameStats(f64 alpha = 0.1) : m_alpha(alpha <= 0.0 ? 0.1 : (alpha > 1.0 ? 1.0 : alpha))
    {
    }

    // Feed one frame's duration in milliseconds.
    void add(f64 frame_ms)
    {
        if (frame_ms < 0.0) {
            frame_ms = 0.0;
        }
        m_ema_ms = m_initialized ? m_alpha * frame_ms + (1.0 - m_alpha) * m_ema_ms : frame_ms;
        m_initialized = true;
    }

    [[nodiscard]] f64 average_ms() const { return m_ema_ms; }
    [[nodiscard]] f64 fps() const { return m_ema_ms > 1e-6 ? 1000.0 / m_ema_ms : 0.0; }
    [[nodiscard]] bool over_budget(f64 budget_ms) const { return m_ema_ms > budget_ms; }

private:
    f64 m_alpha = 0.1;
    f64 m_ema_ms = 0.0;
    bool m_initialized = false;
};

} // namespace vyro
