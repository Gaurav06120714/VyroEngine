// VyroEngine — Animation state machine implementation
#include "vyro/animation/StateMachine.hpp"

namespace vyro {

AnimationStateMachine::StateId AnimationStateMachine::add_state(std::string name)
{
    m_states.push_back(std::move(name));
    return static_cast<StateId>(m_states.size() - 1);
}

void AnimationStateMachine::add_transition(StateId from, StateId to, std::string param, f32 threshold)
{
    m_transitions.push_back(Transition{from, to, std::move(param), threshold});
}

void AnimationStateMachine::set_parameter(std::string_view name, f32 value)
{
    m_parameters[std::string(name)] = value;
}

void AnimationStateMachine::set_current(StateId state)
{
    if (state < m_states.size()) {
        m_current = state;
        m_time = 0.0f;
    }
}

void AnimationStateMachine::update(f32 dt)
{
    m_time += dt;

    for (const Transition& t : m_transitions) {
        if (t.from != m_current) {
            continue;
        }
        const auto it = m_parameters.find(t.param);
        if (it != m_parameters.end() && it->second >= t.threshold) {
            m_current = t.to;
            m_time = 0.0f; // reset local time on transition
            break;
        }
    }
}

std::string_view AnimationStateMachine::current_name() const
{
    return m_current < m_states.size() ? std::string_view(m_states[m_current]) : std::string_view{};
}

} // namespace vyro
