//
// Created by genin on 02/02/2026.
//

#pragma once

#include "Commons.hpp"

namespace GEF
{
    struct WindowProps
    {
        std::string title;
        uint32_t width;
        uint32_t height;

        WindowProps(const std::string& title = "GEF Engine",
                    uint32_t width = 1280,
                    uint32_t height = 720)
            : title(title), width(width), height(height)
        {
        }
    };

    class Window
    {
    public:
        enum Mode
        {
            MODE_WINDOWED,
            MODE_MINIMIZED,
            MODE_MAXIMIZED,
            MODE_FULLSCREEN,
            MODE_EXCLUSIVE,
        };

        enum WindowFlags : uint32_t
        {
            NONE = 0,
            FLAGS_MINIMIZED_DISABLED = 1 << 0,
            FLAGS_MAXIMIZED_DISABLED = 1 << 1,
            FLAG_RESIZE_DISABLED = 1 << 2,
            FLAGS_BORDERLESS = 1 << 3,
            FLAGS_ALWAYS_ON_TOP = 1 << 4,
            FLAGS_TRANSPARENT = 1 << 5,
            FLAGS_NO_FOCUS = 1 << 6,
            FLAGS_NO_CLOSE = 1 << 7,
            FLAGS_VISIBLE = 1 << 8,
            FLAGS_NB = 1 << 9,
        };

        enum WindowInitialPosition
        {
            WINDOW_INITIAL_POSITION_ABSOLUTE,
            WINDOW_INITIAL_POSITION_CENTER_PRIMARY_SCREEN,
            WINDOW_INITIAL_POSITION_CENTER_OTHER_SCREEN,
        };

        using EventCallbackFunction = Events::EventBus::EventCallbackFunction;

    public:
        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        [[nodiscard]] virtual unsigned int GetWidth() const = 0;
        virtual void SetWidth(unsigned int width) = 0;

        [[nodiscard]] virtual unsigned int GetHeight() const = 0;
        virtual void SetHeight(unsigned int height) = 0;

        virtual void SetEventCallback(const EventCallbackFunction& callback) =
        0;

        static Window* Create(const WindowProps& props = WindowProps(),
                              Mode mode = Mode::MODE_WINDOWED,
                              uint32_t flags = WindowFlags::NONE);
    };
}