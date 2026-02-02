//
// Created by genin on 31/01/2026.
//

#pragma once

#include "Event.hpp"

namespace GEF::Events
{
    class WindowResizeEvent : public Event
    {
    public:
        EVENT_TYPE("WindowResizeEvent")

        WindowResizeEvent(int width, int height);

        std::string ToString() const override;

        [[nodiscard]] unsigned int GetWidth() const;
        [[nodiscard]] unsigned int GetHeight() const;

    private:
        unsigned int width_;
        unsigned int height_;
    };

    class WindowCloseEvent : public Event
    {
    public:
        EVENT_TYPE("WindowCloseEvent")

        WindowCloseEvent() = default;
    };

    inline WindowResizeEvent::WindowResizeEvent(int width, int height)
        : width_(width), height_(height)
    {
    }

    inline std::string WindowResizeEvent::ToString() const
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << width_ << ", " << height_;
        return ss.str().c_str();
    }

    inline unsigned int WindowResizeEvent::GetWidth() const
    {
        return width_;
    }

    inline unsigned int WindowResizeEvent::GetHeight() const
    {
        return height_;
    }
}