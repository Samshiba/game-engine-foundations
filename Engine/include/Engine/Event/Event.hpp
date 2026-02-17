//
// Created by genin on 31/01/2026.
// Path: Engine/include/Engine/Event/Event.hpp
//

#pragma once

#include <string>
#include <sstream>
#include <algorithm>

#include <Engine/Utils/FNV-1a.hpp>


namespace GEF::Events
{
    class Event
    {
    public:
        using EventType = std::uint32_t;

        Event() = default;
        virtual ~Event() = default;

        [[nodiscard]] virtual std::string ToString() const;

        [[nodiscard]] virtual EventType GetEventType() const = 0;

        [[nodiscard]] virtual const char* GetName() const = 0;

        [[nodiscard]] virtual bool IsHandled() const;
        void MarkHandled();

    private:
        bool handled_ = false;
    };

    inline bool Event::IsHandled() const
    {
        return handled_;
    }

    inline std::string Event::ToString() const
    {
        std::stringstream ss;
        ss << GetName();
        return ss.str();
    }

    inline void Event::MarkHandled()
    {
        handled_ = true;
    }

    inline std::ostream& operator<<(std::ostream& os, const Event& e)
    {
        return os << e.ToString();
    }

#define EVENT_TYPE(event_type)  static EventType GetStaticEventType() { return GEF::Utils::HashFNV1a(event_type); } \
                                EventType GetEventType() const override { return GetStaticEventType(); } \
                                virtual const char* GetName() const override { return #event_type; }
}