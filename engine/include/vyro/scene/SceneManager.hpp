// VyroEngine — Scene manager
// Phase 2.4: owns scenes and the set of currently active scenes. Supports a
// single primary scene plus additively-loaded scenes, all updated in order.
#pragma once

#include "vyro/core/Types.hpp"
#include "vyro/scene/Scene.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace vyro {

class SceneManager : NonCopyable
{
public:
    SceneManager() = default;

    // Create and own a new scene. It is not active until set/added.
    Scene& create_scene(std::string name);

    // Make `scene` the sole active scene (clears any additive scenes).
    void set_active(Scene& scene);

    // Add `scene` to the active set on top of the current ones (additive load).
    void add_active(Scene& scene);

    // Remove a scene from the active set without destroying it.
    void deactivate(Scene& scene);

    // Destroy a scene; it is removed from the active set first.
    void unload(Scene& scene);

    // Update all active scenes in activation order.
    void update(f32 dt);

    [[nodiscard]] Scene* primary_active() const;
    [[nodiscard]] usize active_count() const { return m_active.size(); }
    [[nodiscard]] usize scene_count() const { return m_scenes.size(); }
    [[nodiscard]] Scene* find(std::string_view name) const;

private:
    std::vector<std::unique_ptr<Scene>> m_scenes; // ownership
    std::vector<Scene*> m_active;                  // activation order
};

} // namespace vyro
