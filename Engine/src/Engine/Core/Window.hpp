//
// Created by genin on 02/02/2026.
//

#pragma once

#include "Commons.hpp"

namespace GEF
{
    class Window
    {
    public:
        enum class WindowMode
        {
            WINDOWED,
            MINIMIZED,
            MAXIMIZED,
            FULLSCREEN,
            EXCLUSIVE,
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
        };

        enum class WindowInitialPosition
        {
            ABSOLUTE,
            CENTER_PRIMARY_SCREEN,
            CENTER_OTHER_SCREEN,
            CENTER_MONITOR_INDEX,
        };

        struct WindowProps
        {
            std::string title;
            uint32_t width;
            uint32_t height;
            WindowMode mode;
            WindowInitialPosition position;
            uint32_t flags;

            WindowProps(const std::string& title = "GEF Engine",
                        uint32_t width = 1280, uint32_t height = 720,
                        WindowMode mode = WindowMode::WINDOWED,
                        WindowInitialPosition position =
                            WindowInitialPosition::CENTER_PRIMARY_SCREEN,
                        uint32_t flags = FLAGS_VISIBLE)
                : title(title), width(width), height(height), mode(mode),
                  position(position),
                  flags(flags)
            {
            }
        };

        using EventCallbackFunction = Events::EventBus::EventCallbackFunction;

    public:
        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        [[nodiscard]] virtual uint32_t GetWidth() const = 0;
        [[nodiscard]] virtual uint32_t GetHeight() const = 0;

        virtual void SetWidth(uint32_t width) = 0;
        virtual void SetHeight(uint32_t height) = 0;

        virtual void SetVSync(bool enabled) = 0;
        [[nodiscard]] virtual bool IsVSync() const = 0;

        virtual void SetEventCallback(const EventCallbackFunction& callback) =
        0;

        [[nodiscard]] virtual void* GetNativeWindow() const = 0;

        static Window* Create(const WindowProps& props = WindowProps());
    };
}