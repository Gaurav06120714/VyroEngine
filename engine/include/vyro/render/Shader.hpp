// VyroEngine — Shader system
// Phase 3.3: a named wrapper around an RHI shader handle plus a caching
// library. Programs are created through the device (compiled offline to SPIR-V
// in real backends) and resolved by name. Hot reload re-creates in place.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/render/RHI.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace vyro {

class Shader
{
public:
    Shader() = default;
    Shader(std::string name, ShaderHandle handle) : m_name(std::move(name)), m_handle(handle) {}

    [[nodiscard]] ShaderHandle handle() const { return m_handle; }
    [[nodiscard]] const std::string& name() const { return m_name; }
    [[nodiscard]] bool valid() const { return m_handle.valid(); }

private:
    std::string m_name;
    ShaderHandle m_handle;
};

class ShaderLibrary : NonCopyable
{
public:
    explicit ShaderLibrary(IRenderDevice& device) : m_device(&device) {}

    // Create and cache a shader under `name`. If it already exists, the cached
    // instance is returned unchanged.
    Shader& load(std::string_view name, const ShaderDesc& desc);

    // Replace an existing shader's program in place (hot reload). No-op if
    // absent.
    void reload(std::string_view name, const ShaderDesc& desc);

    [[nodiscard]] Shader* get(std::string_view name);
    [[nodiscard]] bool exists(std::string_view name) const;
    [[nodiscard]] usize count() const { return m_shaders.size(); }

private:
    IRenderDevice* m_device;
    std::unordered_map<std::string, Shader> m_shaders;
};

} // namespace vyro
