//
// Path: tests/Event/EventBus.test.cpp
//
// Pure CPU. TestEvent / OtherEvent are local events so the tests don't depend
// on the engine's own event types.
//

#include <Engine/Event/EventBus.hpp>

#include <doctest.h>

#include <vector>

using GEF::Events::Event;
using GEF::Events::EventBus;

namespace
{
    struct TestEvent : Event
    {
        EVENT_TYPE("TestEvent")

        explicit TestEvent(int v) : value(v) {}
        int value;
    };

    struct OtherEvent : Event
    {
        EVENT_TYPE("OtherEvent")
    };
} // namespace

TEST_CASE("TriggerEvent calls the subscribers of that event type only")
{
    EventBus bus;
    int testCalls = 0;
    int otherCalls = 0;
    bus.Subscribe(TestEvent::GetStaticEventType(), [&](Event&) { ++testCalls; });
    bus.Subscribe(OtherEvent::GetStaticEventType(), [&](Event&) { ++otherCalls; });

    TestEvent event(1);
    bus.TriggerEvent(event);

    CHECK(testCalls == 1);
    CHECK(otherCalls == 0);
}

TEST_CASE("A queued event is delivered on DispatchEvent, not before")
{
    EventBus bus;
    std::vector<int> received;
    bus.Subscribe(TestEvent::GetStaticEventType(), [&](Event& e) {
        received.push_back(static_cast<TestEvent&>(e).value);
    });

    bus.QueueEvent<TestEvent>(1);
    bus.QueueEvent<TestEvent>(2);
    CHECK(received.empty());

    bus.DispatchEvent();
    CHECK(received == std::vector<int>{ 1, 2 });
}

TEST_CASE("DispatchEvent empties the queue")
{
    EventBus bus;
    int calls = 0;
    bus.Subscribe(TestEvent::GetStaticEventType(), [&](Event&) { ++calls; });

    bus.QueueEvent<TestEvent>(1);
    bus.DispatchEvent();
    bus.DispatchEvent();

    CHECK(calls == 1);
}

TEST_CASE("An event queued during a dispatch is delivered in the same dispatch")
{
    EventBus bus;
    int otherCalls = 0;
    bus.Subscribe(TestEvent::GetStaticEventType(),
                  [&](Event&) { bus.QueueEvent<OtherEvent>(); });
    bus.Subscribe(OtherEvent::GetStaticEventType(), [&](Event&) { ++otherCalls; });

    bus.QueueEvent<TestEvent>(1);
    bus.DispatchEvent();

    CHECK(otherCalls == 1);
}

TEST_CASE("An unsubscribed callback is no longer called")
{
    EventBus bus;
    int calls = 0;
    const auto id = bus.Subscribe(TestEvent::GetStaticEventType(),
                                  [&](Event&) { ++calls; });
    bus.Unsubscribe(TestEvent::GetStaticEventType(), id);

    TestEvent event(1);
    bus.TriggerEvent(event);

    CHECK(calls == 0);
}
