//
// Created by genin on 02/02/2026.
//

#include <Engine/Platform/OpenGL/OpenGlContext.hpp>
#include <Engine/Event/ApplicationEvent.hpp>

#include "WindowsWindow.hpp"
#include "Commons.hpp"

namespace GEF
{
    Window* Window::Create(const WindowProps& props)
    {
        return new Platform::WindowsWindow(props);
    }
}

namespace GEF::Platform
{
    WindowsWindow::WindowsWindow(const WindowProps& props)
    {
        Init(props);
    }

    WindowsWindow::~WindowsWindow()
    {
        Shutdown();
    }

    void WindowsWindow::OnUpdate()
    {
        glfwPollEvents();
        context_->SwapBuffers();
    }

    uint32_t WindowsWindow::GetWidth() const
    {
        return windowData_.width;
    }

    uint32_t WindowsWindow::GetHeight() const
    {
        return windowData_.height;
    }

    void WindowsWindow::SetWidth(uint32_t width)
    {
        windowData_.width = width;
    }

    void WindowsWindow::SetHeight(uint32_t height)
    {
        windowData_.height = height;
    }

    void WindowsWindow::SetVSync(bool enabled)
    {
        if (enabled)
        {
            GEF_ENGINE_DEBUG("VSync enabled");
            glfwSwapInterval(1);
        }
        else
            glfwSwapInterval(0);
        windowData_.VSync = enabled;
    }

    bool WindowsWindow::IsVSync() const
    {
        return windowData_.VSync;
    }

    void WindowsWindow::SetEventCallback(const EventCallbackFunction& callback)
    {
        windowData_.eventCallback = callback;
    }

    void* WindowsWindow::GetNativeWindow() const
    {
        return window_;
    }

    void WindowsWindow::Init(const WindowProps& props)
    {
        GEF_ENGINE_INFO("Creating window : {} ({} x {})", props.title,
                        props.width, props.height);

        windowData_.title = props.title;
        windowData_.width = props.width;
        windowData_.height = props.height;
        windowData_.mode = props.mode;
        windowData_.flags = props.flags;
        windowData_.VSync = true;

        static bool s_GLFWInitialized = false;
        if (!s_GLFWInitialized)
        {
            int success = glfwInit();
            // TODO ASSERT SUCCESS
            if (!success)
            {
                GEF_ENGINE_ERROR("Failed to initialize GLFW");
                return;
            }
            glfwSetErrorCallback(ErrorCallback);
            s_GLFWInitialized = true;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef GEF_DEBUG_BUILD
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

        GLFWmonitor* monitor = nullptr;
        if (windowData_.mode == WindowMode::FULLSCREEN)
        {
            monitor = glfwGetPrimaryMonitor();
        }

        GLFWwindow* window =
            glfwCreateWindow(static_cast<int>(props.width),
                             static_cast<int>(props.height),
                             props.title.c_str(),
                             monitor, nullptr);
        if (!window)
        {
            GEF_ENGINE_ERROR("Failed to create GLFW window");
            glfwTerminate();
            return;
        }
        window_ = window;

        context_ = Renderer::GraphicsContext::Create(window_, props.backend);
        if (!context_)
        {
            GEF_ENGINE_ERROR("Failed to create Graphics Context");
            glfwDestroyWindow(window_);
            glfwTerminate();
            return;
        }
        context_->Init();

        glfwSetWindowUserPointer(window_, &windowData_);
        SetVSync(true);

        glfwSetWindowSizeCallback(window_, WindowResizeCallback);
        glfwSetWindowCloseCallback(window_, WindowCloseCallback);
        glfwSetKeyCallback(window_, KeyCallback);
        glfwSetMouseButtonCallback(window_, MouseButtonCallback);
        glfwSetCursorPosCallback(window_, CursorPositionCallback);
        glfwSetScrollCallback(window_, ScrollCallback);
    }

    void WindowsWindow::Shutdown()
    {
        // TODO REFACTO TO HANDLE MULTI WINDOWS
        GEF_ENGINE_WARN("Destroying windows");
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    void WindowsWindow::ErrorCallback(int error, const char* description)
    {
        GEF_ENGINE_ERROR("GLFW Error : ({}) : {}", error, description);
    }

    void WindowsWindow::WindowResizeCallback(GLFWwindow* window, int width,
                                             int height)
    {
        auto& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        data.width = width;
        data.height = height;

        Events::WindowResizeEvent event(width, height);
        data.eventCallback(event);
    }

    void WindowsWindow::WindowCloseCallback(GLFWwindow* window)
    {
        auto const& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        Events::WindowCloseEvent event;
        data.eventCallback(event);
    }

    void WindowsWindow::KeyCallback(GLFWwindow* window, int key, int scancode,
                                    int action, int mods)
    {
        // TODO SUPPORT MODS AND SCANCODE
        (void)scancode;
        (void)mods;

        auto const& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        switch (action)
        {
        case GLFW_PRESS: {
            Events::KeyPressedEvent event(key, 0);
            data.eventCallback(event);
            break;
        }
        case GLFW_RELEASE: {
            Events::KeyReleasedEvent event(key);
            data.eventCallback(event);
            break;
        }
        case GLFW_REPEAT: {
            Events::KeyPressedEvent event(key, 1);
            data.eventCallback(event);
            break;
        }
        default: {
            GEF_ENGINE_ERROR("Unknown KeyCallback action type {}", action);
        }
        }
    }

    void WindowsWindow::MouseButtonCallback(GLFWwindow* window, int button,
                                            int action, int mods)
    {
        // TODO SUPPORT MODS
        (void)mods;

        auto const& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        switch (action)
        {
        case GLFW_PRESS: {
            Events::MouseButtonPressedEvent event(button);
            data.eventCallback(event);
            break;
        }
        case GLFW_RELEASE: {
            Events::MouseButtonReleasedEvent event(button);
            data.eventCallback(event);
            break;
        }
        default: {
            GEF_ENGINE_ERROR("Unknown MouseButtonCallback action type {}",
                             action);
        }
        }
    }

    void WindowsWindow::CursorPositionCallback(GLFWwindow* window, double xPos,
                                               double yPos)
    {
        auto const& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        Events::MouseMovedEvent event(xPos, yPos);
        data.eventCallback(event);
    }

    void WindowsWindow::ScrollCallback(GLFWwindow* window, double xOffset,
                                       double yOffset)
    {
        auto const& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        Events::MouseScrolledEvent event(xOffset, yOffset);
        data.eventCallback(event);
    }
}