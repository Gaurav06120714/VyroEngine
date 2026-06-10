// VyroEngine — Event System
// Phase 1.4: a type-safe publish/subscribe bus. Subsystems communicate
// without direct references. Events dispatch immediately (publish) or are
// queued for end-of-frame processing (queue + dispatch_queued).
#pragma once

#include "vyro/core/Types.hpp"

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace vyro {

using SubscriptionId = u64;

inline constexpr SubscriptionId kInvalidSubscription = 0;

class EventBus : NonCopyable
{
public:
    EventBus() = default;
    ~EventBus() = default;

    // Register a handler for events of type EventT. Returns an id used to
    // unsubscribe. Handlers are invoked in registration order.
    template<typename EventT>
    SubscriptionId subscribe(std::function<void(const EventT&)> handler)
    {
        const std::type_index key(typeid(EventT));
        const SubscriptionId id = ++m_next_id;
        m_handlers[key].push_back(Entry{
            id,
            [fn = std::move(handler)](const void* payload) {
                fn(*static_cast<const EventT*>(payload));
            },
        });
        return id;
    }

    // Dispatch an event immediately to all current subscribers.
    template<typename EventT>
    void publish(const EventT& event) const
    {
        const std::type_index key(typeid(EventT));
        const auto it = m_handlers.find(key);
        if (it == m_handlers.end()) {
            return;
        }
        for (const Entry& entry : it->second) {
            entry.callback(&event);
        }
    }

    // Defer an event for later batch dispatch via dispatch_queued().
    template<typename EventT>
    void queue(EventT event)
    {
        m_queued.push_back([this, ev = std::move(event)]() { publish(ev); });
    }

    // Invoke all queued events in FIFO order, then clear the queue.
    void dispatch_queued();

    // Remove a previously registered handler. Returns true if found.
    bool unsubscribe(SubscriptionId id);

    // Drop all handlers and queued events.
    void clear();

    [[nodiscard]] usize queued_count() const { return m_queued.size(); }

private:
    struct Entry {
        SubscriptionId id;
        std::function<void(const void*)> callback;
    };

    std::unordered_map<std::type_index, std::vector<Entry>> m_handlers;
    std::vector<std::function<void()>> m_queued;
    SubscriptionId m_next_id = kInvalidSubscription;
};

} // namespace vyro
