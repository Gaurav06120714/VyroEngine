// VyroEngine — Scene manager implementation
#include "vyro/scene/SceneManager.hpp"

#include <algorithm>

namespace vyro {

Scene& SceneManager::create_scene(std::string name)
{
    m_scenes.push_back(std::make_unique<Scene>(std::move(name)));
    return *m_scenes.back();
}

void SceneManager::set_active(Scene& scene)
{
    m_active.clear();
    m_active.push_back(&scene);
}

void SceneManager::add_active(Scene& scene)
{
    if (std::find(m_active.begin(), m_active.end(), &scene) == m_active.end()) {
        m_active.push_back(&scene);
    }
}

void SceneManager::deactivate(Scene& scene)
{
    m_active.erase(std::remove(m_active.begin(), m_active.end(), &scene), m_active.end());
}

void SceneManager::unload(Scene& scene)
{
    deactivate(scene);
    m_scenes.erase(
        std::remove_if(m_scenes.begin(), m_scenes.end(),
                       [&](const std::unique_ptr<Scene>& s) { return s.get() == &scene; }),
        m_scenes.end());
}

void SceneManager::update(f32 dt)
{
    for (Scene* scene : m_active) {
        scene->update(dt);
    }
}

Scene* SceneManager::primary_active() const
{
    return m_active.empty() ? nullptr : m_active.front();
}

Scene* SceneManager::find(std::string_view name) const
{
    for (const auto& scene : m_scenes) {
        if (scene->name() == name) {
            return scene.get();
        }
    }
    return nullptr;
}

} // namespace vyro
