//
// Created by genin on 02/02/2026.
//

#include "WindowsWindow.hpp"
#include "Commons.hpp"

#include <Engine/Platform/OpenGL/OpenGlContext.hpp>
#include <Engine/Event/ApplicationEvent.hpp>

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
            glfwSwapInterval(1);
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

        // TODO REFACTO TO NOT HARDCODE OPENGL CONTEXT
        context_ = std::make_unique<OpenGLContext>(window_);
        context_->Init();

        glfwSetWindowUserPointer(window_, &windowData_);
        SetVSync(true);

        glfwSetWindowSizeCallback(window_, WindowResizeCallback);
        // glfwSetWindowCloseCallback(window_, WindowCloseCallback);
        // glfwSetKeyCallback(window_, KeyCallback);
        // glfwSetMouseButtonCallback(window_, MouseButtonCallback);
        // glfwSetCursorPosCallback(window_, CursorPositionCallback);
        // glfwSetScrollCallback(window_, ScrollCallback);
    }

    void WindowsWindow::Shutdown()
    {
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

    // void WindowsWindow::WindowCloseCallback(GLFWwindow* window)
    // {
    //     // TODO
    // }
    //
    // void WindowsWindow::KeyCallback(GLFWwindow* window, int key, int scancode,
    //                                 int action, int mods)
    // {
    //     // TODO
    // }
    //
    // void WindowsWindow::MouseButtonCallback(GLFWwindow* window, int button,
    //                                         int action, int mods)
    // {
    //     // TODO
    // }
    //
    // void WindowsWindow::CursorPositionCallback(GLFWwindow* window, double xPos,
    //                                            double yPos)
    // {
    //     // TODO
    // }
    //
    // void WindowsWindow::ScrollCallback(GLFWwindow* window, double xOffset,
    //                                    double yOffset)
    // {
    //     // TODO
    // }
}