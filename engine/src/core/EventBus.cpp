// VyroEngine — Event System implementation
#include "vyro/core/EventBus.hpp"

namespace vyro {

void EventBus::dispatch_queued()
{
    // Swap out the queue first so handlers that enqueue new events do not
    // grow the vector we are iterating (those run on the next dispatch).
    std::vector<std::function<void()>> pending;
    pending.swap(m_queued);
    for (const auto& invoke : pending) {
        invoke();
    }
}

bool EventBus::unsubscribe(SubscriptionId id)
{
    for (auto& [key, entries] : m_handlers) {
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            if (it->id == id) {
                entries.erase(it);
                return true;
            }
        }
    }
    return false;
}

void EventBus::clear()
{
    m_handlers.clear();
    m_queued.clear();
}

} // namespace vyro
