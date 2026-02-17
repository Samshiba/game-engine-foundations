//
// Created by genin on 31/01/2026.
// Path: Engine/include/Engine/Event/ApplicationEvent.hpp
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

        [[nodiscard]] std::string ToString() const override;

        [[nodiscard]] unsigned int GetWidth() const;
        [[nodiscard]] unsigned int GetHeight() const;

    private:
        unsigned int width_;
        unsigned int height_;
    };

    // Window Events
    class WindowCloseEvent : public Event
    {
    public:
        EVENT_TYPE("WindowCloseEvent")

        WindowCloseEvent() = default;
    };

    // Key Events
    class KeyPressedEvent : public Event
    {
    public:
        EVENT_TYPE("KeyPressedEvent")

        KeyPressedEvent(int key, int repeat);

        [[nodiscard]] std::string ToString() const override;

        [[nodiscard]] int GetKey() const;
        [[nodiscard]] bool IsHeld() const;

    private:
        int key_;
        int repeat_;
    };

    class KeyReleasedEvent : public Event
    {
    public:
        EVENT_TYPE("KeyReleasedEvent")

        KeyReleasedEvent(int key);

        [[nodiscard]] std::string ToString() const override;

        [[nodiscard]] int GetKey() const;

    private:
        int key_;
    };

    // Mouse Button Events
    class MouseButtonPressedEvent : public Event
    {
    public:
        EVENT_TYPE("MouseButtonPressedEvent")

        MouseButtonPressedEvent(int button);

        [[nodiscard]] std::string ToString() const override;

        [[nodiscard]] int GetButton() const;

    private:
        int button_;
    };

    class MouseButtonReleasedEvent : public Event
    {
    public:
        EVENT_TYPE("MouseButtonReleasedEvent")

        MouseButtonReleasedEvent(int button);

        [[nodiscard]] std::string ToString() const override;

        [[nodiscard]] int GetButton() const;

    private:
        int button_;
    };

    // Mouse Move Event
    class MouseMovedEvent : public Event
    {
    public:
        EVENT_TYPE("MouseMovedEvent")

        MouseMovedEvent(double x, double y);

        [[nodiscard]] std::string ToString() const override;

        [[nodiscard]] double GetX() const;
        [[nodiscard]] double GetY() const;

    private:
        double x_;
        double y_;
    };

    class MouseScrolledEvent : public Event
    {
    public:
        EVENT_TYPE("MouseScrolledEvent")

        MouseScrolledEvent(double xOffset, double yOffset);

        [[nodiscard]] std::string ToString() const override;

        [[nodiscard]] double GetXOffset() const;
        [[nodiscard]] double GetYOffset() const;

    private:
        double xOffset_;
        double yOffset_;
    };
}

#include "ApplicationEvent.hxx"