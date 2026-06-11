// VyroEngine — Lua scripting backend (V2.3)
// A real Lua VM behind the same surface as ScriptEngine: numeric globals,
// run/eval, host functions, and reset. Gameplay code written against either
// engine is interchangeable; VyroScript remains the dependency-free fallback.
#pragma once

#include "vyro/core/Types.hpp"

#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct lua_State;

namespace vyro {

enum class LuaError {
    LoadFailed,
    RuntimeError,
};

class LuaScriptEngine
{
public:
    using HostFunction = std::function<f64(const std::vector<f64>&)>;

    LuaScriptEngine();
    ~LuaScriptEngine();

    LuaScriptEngine(const LuaScriptEngine&) = delete;
    LuaScriptEngine& operator=(const LuaScriptEngine&) = delete;

    // Register a host function callable from Lua as a global.
    void register_function(std::string_view name, HostFunction fn);

    // Execute a chunk of Lua source.
    std::expected<void, LuaError> run(std::string_view source);

    // Evaluate a Lua expression and return its numeric result.
    std::expected<f64, LuaError> eval(std::string_view expression);

    [[nodiscard]] f64 get(std::string_view name) const;
    void set(std::string_view name, f64 value);
    [[nodiscard]] bool has(std::string_view name) const;

    // Recreate the VM with fresh globals; host functions are re-registered.
    void reset();

    // Last error message from Lua (empty when the previous call succeeded).
    [[nodiscard]] const std::string& last_error() const { return m_last_error; }

private:
    void open_state();
    void install_function(const std::string& name);

    lua_State* m_lua = nullptr;
    std::unordered_map<std::string, HostFunction> m_functions;
    std::string m_last_error;
};

} // namespace vyro
