// VyroEngine — Lua scripting backend implementation
#include "vyro/scripting/LuaScriptEngine.hpp"

#include <lua.hpp>

namespace vyro {

namespace {

// Trampoline: a lua_CFunction whose upvalue is the HostFunction pointer.
// Collects numeric arguments, invokes the host, pushes the numeric result.
int host_trampoline(lua_State* L)
{
    auto* fn = static_cast<LuaScriptEngine::HostFunction*>(
        lua_touserdata(L, lua_upvalueindex(1)));

    const int argc = lua_gettop(L);
    std::vector<f64> args;
    args.reserve(static_cast<usize>(argc));
    for (int i = 1; i <= argc; ++i) {
        args.push_back(lua_tonumber(L, i));
    }
    lua_pushnumber(L, (*fn)(args));
    return 1;
}

} // namespace

LuaScriptEngine::LuaScriptEngine()
{
    open_state();
}

LuaScriptEngine::~LuaScriptEngine()
{
    if (m_lua != nullptr) {
        lua_close(m_lua);
    }
}

void LuaScriptEngine::open_state()
{
    m_lua = luaL_newstate();
    luaL_openlibs(m_lua);
}

void LuaScriptEngine::install_function(const std::string& name)
{
    lua_pushlightuserdata(m_lua, &m_functions[name]);
    lua_pushcclosure(m_lua, host_trampoline, 1);
    lua_setglobal(m_lua, name.c_str());
}

void LuaScriptEngine::register_function(std::string_view name, HostFunction fn)
{
    const std::string key(name);
    m_functions[key] = std::move(fn);
    install_function(key);
}

std::expected<void, LuaError> LuaScriptEngine::run(std::string_view source)
{
    m_last_error.clear();
    if (luaL_loadbuffer(m_lua, source.data(), source.size(), "chunk") != LUA_OK) {
        m_last_error = lua_tostring(m_lua, -1);
        lua_pop(m_lua, 1);
        return std::unexpected(LuaError::LoadFailed);
    }
    if (lua_pcall(m_lua, 0, 0, 0) != LUA_OK) {
        m_last_error = lua_tostring(m_lua, -1);
        lua_pop(m_lua, 1);
        return std::unexpected(LuaError::RuntimeError);
    }
    return {};
}

std::expected<f64, LuaError> LuaScriptEngine::eval(std::string_view expression)
{
    m_last_error.clear();
    const std::string chunk = "return " + std::string(expression);
    if (luaL_loadbuffer(m_lua, chunk.data(), chunk.size(), "eval") != LUA_OK) {
        m_last_error = lua_tostring(m_lua, -1);
        lua_pop(m_lua, 1);
        return std::unexpected(LuaError::LoadFailed);
    }
    if (lua_pcall(m_lua, 0, 1, 0) != LUA_OK) {
        m_last_error = lua_tostring(m_lua, -1);
        lua_pop(m_lua, 1);
        return std::unexpected(LuaError::RuntimeError);
    }
    const f64 value = lua_tonumber(m_lua, -1);
    lua_pop(m_lua, 1);
    return value;
}

f64 LuaScriptEngine::get(std::string_view name) const
{
    lua_getglobal(m_lua, std::string(name).c_str());
    const f64 value = lua_tonumber(m_lua, -1);
    lua_pop(m_lua, 1);
    return value;
}

void LuaScriptEngine::set(std::string_view name, f64 value)
{
    lua_pushnumber(m_lua, value);
    lua_setglobal(m_lua, std::string(name).c_str());
}

bool LuaScriptEngine::has(std::string_view name) const
{
    lua_getglobal(m_lua, std::string(name).c_str());
    const bool present = lua_isnil(m_lua, -1) == 0;
    lua_pop(m_lua, 1);
    return present;
}

void LuaScriptEngine::reset()
{
    lua_close(m_lua);
    open_state();
    // Re-install host functions into the fresh VM.
    for (const auto& [name, fn] : m_functions) {
        install_function(name);
    }
}

} // namespace vyro
