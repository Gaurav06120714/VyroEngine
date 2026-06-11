// VyroEngine — Script host implementation
#include "vyro/scripting/ScriptHost.hpp"

#include "vyro/core/Log.hpp"

#include <fstream>
#include <sstream>

namespace vyro {

bool ScriptHost::load()
{
    std::ifstream file(m_path);
    if (!file.is_open()) {
        VYRO_WARN("Script", "script not found: {}", m_path);
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::error_code ec;
    m_mtime = std::filesystem::last_write_time(m_path, ec);

    m_engine.reset(); // fresh globals; host functions persist
    const auto result = m_engine.run(buffer.str());
    if (!result.has_value()) {
        VYRO_ERROR("Script", "script error in {}", m_path);
        return false;
    }
    return true;
}

bool ScriptHost::poll()
{
    std::error_code ec;
    const auto mtime = std::filesystem::last_write_time(m_path, ec);
    if (ec || mtime == m_mtime) {
        return false;
    }
    VYRO_INFO("Script", "hot-reloading {}", m_path);
    ++m_reloads;
    return load();
}

} // namespace vyro
