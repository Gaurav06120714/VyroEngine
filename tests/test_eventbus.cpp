// VyroEngine — EventBus tests
#include "vyro/core/EventBus.hpp"

#include "test_harness.hpp"

namespace {

struct DamageEvent {
    int amount = 0;
};

struct HealEvent {
    int amount = 0;
};

} // namespace

int main()
{
    vyro::test::Suite suite("eventbus");

    // EventBus_Publish_InvokesMatchingHandler
    {
        vyro::EventBus bus;
        int received = 0;
        bus.subscribe<DamageEvent>([&](const DamageEvent& e) { received += e.amount; });
        bus.publish(DamageEvent{.amount = 10});
        bus.publish(DamageEvent{.amount = 5});
        suite.check(received == 15, "matching handler accumulates 15");
    }

    // EventBus_Publish_IgnoresUnrelatedType
    {
        vyro::EventBus bus;
        int damage = 0;
        bus.subscribe<DamageEvent>([&](const DamageEvent& e) { damage += e.amount; });
        bus.publish(HealEvent{.amount = 99});
        suite.check(damage == 0, "unrelated event type ignored");
    }

    // EventBus_Unsubscribe_StopsDelivery
    {
        vyro::EventBus bus;
        int count = 0;
        auto id = bus.subscribe<DamageEvent>([&](const DamageEvent&) { ++count; });
        bus.publish(DamageEvent{});
        suite.check(bus.unsubscribe(id), "unsubscribe returns true for valid id");
        bus.publish(DamageEvent{});
        suite.check(count == 1, "handler not called after unsubscribe");
        suite.check(!bus.unsubscribe(id), "double unsubscribe returns false");
    }

    // EventBus_Queue_DefersUntilDispatch
    {
        vyro::EventBus bus;
        int total = 0;
        bus.subscribe<DamageEvent>([&](const DamageEvent& e) { total += e.amount; });
        bus.queue(DamageEvent{.amount = 7});
        bus.queue(DamageEvent{.amount = 3});
        suite.check(total == 0, "queued events not delivered before dispatch");
        suite.check(bus.queued_count() == 2, "two events queued");
        bus.dispatch_queued();
        suite.check(total == 10, "dispatch delivers queued events");
        suite.check(bus.queued_count() == 0, "queue empty after dispatch");
    }

    return suite.summary();
}
