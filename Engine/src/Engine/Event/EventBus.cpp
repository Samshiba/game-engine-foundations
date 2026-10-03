//
// Created by genin on 31/01/2026.
// Path: Engine/src/Engine/Event/EventBus.cpp
//

#include <Engine/Event/EventBus.hpp>
#include <Engine/Core/Log.hpp>

namespace GEF::Events
{
    EventBus::SubscriberID EventBus::Subscribe(EventType eventType,
                                               const EventCallbackFunction&
                                               callback)
    {
        SubscriberID id = nextSubscriberId_++;
        subscribers_[eventType].emplace_back(id, callback);
        return id;
    }

    void EventBus::Unsubscribe(EventType eventType, SubscriberID subscriberID)
    {
        if (!subscribers_.contains(eventType))
            return;

        auto& subscribers = subscribers_[eventType];
        std::erase_if(subscribers,
                      [subscriberID](const Subscriber& s) {
                          return subscriberID == s.id;
                      });
    }

    void EventBus::TriggerEvent(Event& event)
    {
        const auto it = subscribers_.find(event.GetEventType());
        if (it == subscribers_.end())
            return;

        for (auto const& subscriber : it->second)
        {
            subscriber.callback(event);
        }
    }

    void EventBus::DispatchEvent()
    {
        while (!eventQueue_.empty())
        {
            auto const& event = eventQueue_.front();
            TriggerEvent(*event);
            eventQueue_.pop_front();
        }
    }
}