// VyroEngine — Data-driven wave plans (V8.5)
// A wave is `kills,intermission` per line; a level's pacing is authored as text
// (file or embedded) instead of a hard-coded ramp. Tolerant parse, headless and
// tested. GameFlow consumes the parsed list.
#pragma once

#include "vyro/core/Types.hpp"

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace vyro::game {

struct WavePlan {
    int kills = 5;            // kills to clear this wave
    f32 intermission = 3.0f;  // pause before the next wave (seconds)
};

// Parse "kills,intermission" lines. Lines without a comma or a positive kill
// count are skipped; '#' starts a comment. Returns an empty vector if nothing
// valid is found (caller falls back to a default).
[[nodiscard]] inline std::vector<WavePlan> parse_wave_plans(std::string_view text)
{
    std::vector<WavePlan> out;
    usize pos = 0;
    while (pos < text.size()) {
        const usize eol = text.find('\n', pos);
        std::string_view line =
            text.substr(pos, eol == std::string_view::npos ? text.size() - pos : eol - pos);
        pos = eol == std::string_view::npos ? text.size() : eol + 1;

        const usize hash = line.find('#');
        if (hash != std::string_view::npos) {
            line = line.substr(0, hash);
        }
        const usize comma = line.find(',');
        if (comma == std::string_view::npos) {
            continue;
        }
        const int kills = std::atoi(std::string(line.substr(0, comma)).c_str());
        const f32 inter = static_cast<f32>(std::atof(std::string(line.substr(comma + 1)).c_str()));
        if (kills > 0) {
            out.push_back(WavePlan{kills, inter < 0.0f ? 0.0f : inter});
        }
    }
    return out;
}

} // namespace vyro::game
