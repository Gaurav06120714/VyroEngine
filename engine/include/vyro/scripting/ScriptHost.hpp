// VyroEngine — Script host with hot reload
// Phase 7.3: owns a ScriptEngine bound to a script file. poll() re-runs the
// script whenever the file's modification time changes, preserving the
// registered host functions while resetting script globals.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/scripting/ScriptEngine.hpp"

#include <filesystem>
#include <string>

namespace vyro {

class ScriptHost
{
public:
    explicit ScriptHost(std::string path) : m_path(std::move(path)) {}

    [[nodiscard]] ScriptEngine& engine() { return m_engine; }

    // Load and run the script now. Returns false if the file is missing or
    // the script fails.
    bool load();

    // Re-run the script if the file changed since the last load.
    // Returns true if a reload happened.
    bool poll();

    [[nodiscard]] u32 reload_count() const { return m_reloads; }

private:
    std::string m_path;
    ScriptEngine m_engine;
    std::filesystem::file_time_type m_mtime{};
    u32 m_reloads = 0;
};

} // namespace vyro
