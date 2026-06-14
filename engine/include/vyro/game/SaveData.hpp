// VyroEngine — Save data & settings (V8.1)
// A tiny persisted profile: best score/wave plus settings (volume, difficulty).
// Serialized as plain `key=value` lines so it is human-readable, diffable, and
// trivially testable; file IO is a thin wrapper over the string form.
#pragma once

#include "vyro/core/Types.hpp"

#include <fstream>
#include <string>
#include <string_view>

namespace vyro::game {

struct SaveData {
    int high_score = 0;
    int best_wave = 0;
    f32 master_volume = 1.0f;
    int difficulty = 1; // 0=easy, 1=normal, 2=hard
    u32 medals = 0;     // V9.5: accumulated medal bitmask
};

[[nodiscard]] inline std::string serialize(const SaveData& s)
{
    std::string out;
    out += "high_score=" + std::to_string(s.high_score) + "\n";
    out += "best_wave=" + std::to_string(s.best_wave) + "\n";
    out += "master_volume=" + std::to_string(s.master_volume) + "\n";
    out += "difficulty=" + std::to_string(s.difficulty) + "\n";
    out += "medals=" + std::to_string(s.medals) + "\n";
    return out;
}

// Tolerant parse: unknown keys are ignored, missing keys keep their defaults,
// and values are clamped to sane ranges.
[[nodiscard]] inline SaveData parse(std::string_view text)
{
    SaveData s;
    usize pos = 0;
    while (pos < text.size()) {
        const usize eol = text.find('\n', pos);
        const std::string_view line =
            text.substr(pos, eol == std::string_view::npos ? text.size() - pos : eol - pos);
        pos = eol == std::string_view::npos ? text.size() : eol + 1;

        const usize eq = line.find('=');
        if (eq == std::string_view::npos) {
            continue;
        }
        const std::string_view key = line.substr(0, eq);
        const std::string value{line.substr(eq + 1)};
        if (key == "high_score") {
            s.high_score = std::atoi(value.c_str());
        } else if (key == "best_wave") {
            s.best_wave = std::atoi(value.c_str());
        } else if (key == "master_volume") {
            s.master_volume = static_cast<f32>(std::atof(value.c_str()));
        } else if (key == "difficulty") {
            s.difficulty = std::atoi(value.c_str());
        } else if (key == "medals") {
            s.medals = static_cast<u32>(std::strtoul(value.c_str(), nullptr, 10));
        }
    }
    if (s.high_score < 0) {
        s.high_score = 0;
    }
    if (s.best_wave < 0) {
        s.best_wave = 0;
    }
    s.master_volume = s.master_volume < 0.0f ? 0.0f : (s.master_volume > 1.0f ? 1.0f : s.master_volume);
    s.difficulty = s.difficulty < 0 ? 0 : (s.difficulty > 2 ? 2 : s.difficulty);
    return s;
}

inline bool save_to_file(const std::string& path, const SaveData& s)
{
    std::ofstream f(path, std::ios::trunc);
    if (!f.is_open()) {
        return false;
    }
    f << serialize(s);
    return true;
}

[[nodiscard]] inline SaveData load_from_file(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        return SaveData{};
    }
    const std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return parse(text);
}

} // namespace vyro::game
