//
// Created by genin on 31/01/2026.
// Path: Engine/include/Engine/Event/EventBus.hpp
//

#pragma once

#include <memory>
#include <utility>
#include <functional>

#include <vector>
#include <deque>
#include <unordered_map>

#include "Event.hpp"


namespace GEF::Events
{
    class EventBus
    {
    public:
        using EventCallbackFunction = std::function<void(Event&)>;
        using SubscriberID = uint32_t;
        using EventType = Event::EventType;

    private:
        struct Subscriber
        {
            SubscriberID id;
            EventCallbackFunction callback;
        };

        std::deque<std::unique_ptr<Event> > eventQueue_;
        std::unordered_map<EventType, std::vector<Subscriber> > subscribers_;
        SubscriberID nextSubscriberId_ = 0;

    public:
        EventBus() = default;
        ~EventBus() = default;

        EventBus(const EventBus&) = delete;
        EventBus& operator=(const EventBus&) = delete;

        SubscriberID Subscribe(EventType eventType,
                               const EventCallbackFunction& callback);
        void Unsubscribe(EventType eventType, SubscriberID subscriberID);

        void TriggerEvent(Event& event);

        template <typename T, typename... Args>
        void QueueEvent(Args&&... args);

        void DispatchEvent();
    };

    template <typename T, typename... Args>
    inline void EventBus::QueueEvent(Args&&... args)
    {
        GEF_ENGINE_DEBUG("EventBus: Queuing event of type {}",
                         typeid(T).name());
        eventQueue_.
            push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }
}