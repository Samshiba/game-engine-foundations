//
// Created by genin on 31/01/2026.
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
        EventType type = event.GetEventType();

        if (subscribers_.contains(type))
        {
            auto const& list = subscribers_[type];
            GEF_ENGINE_INFO("EventBus: Dispatching [{}] to {} subscribers",
                            event.ToString(), list.size());

            for (auto const& subscriber : subscribers_[type])
            {
                subscriber.callback(event);
            }
        }
        else
        {
            GEF_ENGINE_WARN(
                "EventBus: Event [{}] dispatched but no subscribers found!",
                event.ToString());
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