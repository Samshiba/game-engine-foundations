//
// Created by genin on 02/02/2026.
//

#pragma once

#include <Engine/Core/Window.hpp>
#include <Engine/Platform/OpenGL/OpenGlContext.hpp>
#include <Engine/Event/Event.hpp>
#include <GLFW/glfw3.h>

#include "Commons.hpp"

namespace GEF::Platform
{
    class WindowsWindow : public Window
    {
    private:
        using EventCallbackFunction = std::function<void(Events::Event&)>;

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
        ~WindowsWindow();

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
        void Init(const WindowProps& props);
        void Shutdown();

        static void ErrorCallback(int error, const char* description);
        static void WindowResizeCallback(GLFWwindow* window, int width,
                                         int height);
        static void WindowCloseCallback(GLFWwindow* window);
        // static void KeyCallback(GLFWwindow* window, int key, int scancode,
        //                         int action, int mods);
        // static void MouseButtonCallback(GLFWwindow* window, int button,
        //                                 int action, int mods);
        // static void CursorPositionCallback(GLFWwindow* window, double xPos,
        //                                    double yPos);
        // static void ScrollCallback(GLFWwindow* window, double xOffset,
        //                            double yOffset);

    private:
        unsigned int monitor_ = 0; // TODO SUPPORT MULTI MONITOR
        std::unique_ptr<Renderer::GraphicsContext> context_;
        GLFWwindow* window_ = nullptr;
        WindowData windowData_;
    };
}