//
// Created by genin on 02/02/2026.
//

#pragma once


#include <Engine/Core/Window.hpp>
#include <GLFW/glfw3.h>

#include "Commons.hpp"


namespace GEF
{
    class WindowsWindow : public Window
    {
    private:
        struct WindowData
        {
            std::string title;
            uint32_t width;
            uint32_t height;
            bool VSync;

            WindowMode mode;
            uint32_t flags;

            EventCallbackFunction eventCallback;
        };

    public:
        WindowsWindow(const WindowProps& props);

        void OnUpdate() override;

        [[nodiscard]] uint32_t GetWidth() const override;
        [[nodiscard]] uint32_t GetHeight() const override;

        void SetWidth(uint32_t width) override;
        void SetHeight(uint32_t height) override;

        void SetVSync(bool enabled) override;
        [[nodiscard]] bool IsVSync() const override;

        void SetEventCallback(const EventCallbackFunction& callback) override;
        [[nodiscard]] void* GetNativeWindow() const override;

    private:
        virtual void Init(const WindowProps& props);
        virtual void Shutdown();

    private:
        unsigned int monitor_ = 0;
        GLFWwindow* window_ = nullptr;
        WindowData windowData_;
    };
}