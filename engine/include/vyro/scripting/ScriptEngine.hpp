// VyroEngine — Scripting system
// Phase 7.1/7.2: an embedded expression/statement interpreter (VyroScript).
// Supports numeric variables, arithmetic, comparisons, if/while blocks, and
// host functions registered by the engine (the Script API). The interface is
// VM-agnostic so a Lua backend can replace the interpreter later.
//
// Grammar (newline-terminated statements):
//   x = 2 + 3 * 4
//   if x > 10 { y = 1 } else { y = 0 }
//   while x > 0 { x = x - 1 }
//   spawn(x, 0, 0)        # host function call
#pragma once

#include "vyro/core/Types.hpp"

#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vyro {

enum class ScriptError {
    SyntaxError,
    UnknownIdentifier,
    DivisionByZero,
};

class ScriptEngine
{
public:
    using HostFunction = std::function<f64(const std::vector<f64>&)>;

    // Register a host function callable from scripts (Script API surface).
    void register_function(std::string_view name, HostFunction fn);

    // Execute a script. Variables persist between runs (the VM's globals).
    std::expected<void, ScriptError> run(std::string_view source);

    // Evaluate a single expression and return its value.
    std::expected<f64, ScriptError> eval(std::string_view expression);

    [[nodiscard]] f64 get(std::string_view name) const;
    void set(std::string_view name, f64 value);
    [[nodiscard]] bool has(std::string_view name) const;

    // Drop all globals (used by hot reload to re-run a fresh state).
    void reset();

private:
    std::unordered_map<std::string, f64> m_globals;
    std::unordered_map<std::string, HostFunction> m_functions;
};

} // namespace vyro
