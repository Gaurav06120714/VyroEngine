// VyroEngine — Animation state machine
// Phase 6.3: states map to animation clips; transitions fire when a named
// parameter crosses a threshold (e.g. speed > 3 -> Run). The machine advances
// the active state's local time and evaluates transitions each update.
#pragma once

#include "vyro/core/Types.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vyro {

class AnimationStateMachine
{
public:
    using StateId = u32;
    static constexpr StateId kInvalidState = 0xFFFFFFFFu;

    StateId add_state(std::string name);

    // Transition from -> to when parameter `param` >= `threshold`.
    void add_transition(StateId from, StateId to, std::string param, f32 threshold);

    void set_parameter(std::string_view name, f32 value);
    void set_current(StateId state);

    // Advance the active state's time and evaluate outgoing transitions.
    void update(f32 dt);

    [[nodiscard]] StateId current() const { return m_current; }
    [[nodiscard]] std::string_view current_name() const;
    [[nodiscard]] f32 time() const { return m_time; }
    [[nodiscard]] usize state_count() const { return m_states.size(); }

private:
    struct Transition {
        StateId from = kInvalidState;
        StateId to = kInvalidState;
        std::string param;
        f32 threshold = 0.0f;
    };

    std::vector<std::string> m_states;
    std::vector<Transition> m_transitions;
    std::unordered_map<std::string, f32> m_parameters;
    StateId m_current = 0;
    f32 m_time = 0.0f;
};

} // namespace vyro
