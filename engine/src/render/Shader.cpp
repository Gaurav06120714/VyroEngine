// VyroEngine — Shader system implementation
#include "vyro/render/Shader.hpp"

namespace vyro {

Shader& ShaderLibrary::load(std::string_view name, const ShaderDesc& desc)
{
    const std::string key(name);
    auto it = m_shaders.find(key);
    if (it != m_shaders.end()) {
        return it->second;
    }
    const ShaderHandle handle = m_device->create_shader(desc);
    auto [inserted, ok] = m_shaders.emplace(key, Shader(key, handle));
    return inserted->second;
}

void ShaderLibrary::reload(std::string_view name, const ShaderDesc& desc)
{
    const auto it = m_shaders.find(std::string(name));
    if (it == m_shaders.end()) {
        return;
    }
    m_device->destroy_shader(it->second.handle());
    it->second = Shader(it->second.name(), m_device->create_shader(desc));
}

Shader* ShaderLibrary::get(std::string_view name)
{
    const auto it = m_shaders.find(std::string(name));
    return it != m_shaders.end() ? &it->second : nullptr;
}

bool ShaderLibrary::exists(std::string_view name) const
{
    return m_shaders.contains(std::string(name));
}

} // namespace vyro
