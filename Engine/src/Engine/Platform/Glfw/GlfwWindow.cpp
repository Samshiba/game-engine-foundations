//
// Created by genin on 02/02/2026.
// Path: Engine/src/Engine/Platform/Glfw/GlfwWindow.cpp
//

#include <Engine/Platform/OpenGL/OpenGlContext.hpp>
#include <Engine/Event/ApplicationEvent.hpp>

#include "GlfwWindow.hpp"
#include "Commons.hpp"

namespace GEF::Platform
{
    GlfwWindow::GlfwWindow(const WindowProps& props,
                           Renderer::RendererBackend backend)
    {
        Init(props, backend);
    }

    GlfwWindow::~GlfwWindow()
    {
        Shutdown();
    }

    void GlfwWindow::OnUpdate()
    {
        glfwPollEvents();
        context_->SwapBuffers();
    }

    uint32_t GlfwWindow::GetWidth() const
    {
        return windowData_.width;
    }

    uint32_t GlfwWindow::GetHeight() const
    {
        return windowData_.height;
    }

    void GlfwWindow::SetWidth(uint32_t width)
    {
        windowData_.width = width;
    }

    void GlfwWindow::SetHeight(uint32_t height)
    {
        windowData_.height = height;
    }

    void GlfwWindow::SetVSync(bool enabled)
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

    bool GlfwWindow::IsVSync() const
    {
        return windowData_.VSync;
    }

    void GlfwWindow::SetCursorMode(CursorMode mode)
    {
        windowData_.cursorMode = mode;
        switch (mode)
        {
        case CursorMode::NORMAL:
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            GEF_ENGINE_DEBUG("Cursor mode set to NORMAL");
            break;
        case CursorMode::HIDDEN:
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            GEF_ENGINE_DEBUG("Cursor mode set to HIDDEN");
            break;
        case CursorMode::DISABLED:
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported())
            {
                glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }
            GEF_ENGINE_DEBUG("Cursor mode set to DISABLED");
            break;
        default:
            GEF_ENGINE_DEBUG("Cursor mode set to NORMAL");
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    void GlfwWindow::SetEventCallback(const EventCallbackFunction& callback)
    {
        windowData_.eventCallback = callback;
    }

    void* GlfwWindow::GetNativeWindow() const
    {
        return window_;
    }

    void GlfwWindow::Init(const WindowProps& props,
                          Renderer::RendererBackend backend)
    {
        GEF_ENGINE_INFO("Creating window : {} ({} x {})", props.title,
                        props.width, props.height);

        windowData_.title = props.title;
        windowData_.width = props.width;
        windowData_.height = props.height;
        windowData_.windowMode = props.windowMode;
        windowData_.cursorMode = props.cursorMode;
        windowData_.flags = props.flags;
        windowData_.VSync = true;

        static bool s_GLFWInitialized = false;
        if (!s_GLFWInitialized)
        {
            int success = glfwInit();
            GEF_CORE_ASSERT(success, "Failed to initialize GLFW");
            glfwSetErrorCallback(ErrorCallback);
            s_GLFWInitialized = true;
        }

        if (backend != Renderer::RendererBackend::OpenGL)
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef GEF_DEBUG_BUILD
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

        GLFWmonitor* monitor = nullptr;
        if (windowData_.windowMode == WindowMode::FULLSCREEN)
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

        context_ = Renderer::GraphicsContext::Create(window_, backend);
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
        SetCursorMode(windowData_.cursorMode);

        glfwSetWindowSizeCallback(window_, WindowResizeCallback);
        glfwSetWindowCloseCallback(window_, WindowCloseCallback);
        glfwSetKeyCallback(window_, KeyCallback);
        glfwSetMouseButtonCallback(window_, MouseButtonCallback);
        glfwSetCursorPosCallback(window_, CursorPositionCallback);
        glfwSetScrollCallback(window_, ScrollCallback);
    }

    void GlfwWindow::Shutdown()
    {
        // TODO REFACTO TO HANDLE MULTI WINDOWS
        GEF_ENGINE_WARN("Destroying windows");
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    void GlfwWindow::ErrorCallback([[maybe_unused]] int error,
                                   [[maybe_unused]] const char* description)
    {
        GEF_ENGINE_ERROR("GLFW Error : ({}) : {}", error, description);
    }

    void GlfwWindow::WindowResizeCallback(GLFWwindow* window, int width,
                                          int height)
    {
        auto& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        data.width = width;
        data.height = height;

        Events::WindowResizeEvent event(width, height);
        data.eventCallback(event);
    }

    void GlfwWindow::WindowCloseCallback(GLFWwindow* window)
    {
        auto const& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        Events::WindowCloseEvent event;
        data.eventCallback(event);
    }

    void GlfwWindow::KeyCallback(GLFWwindow* window, int key,
                                 [[maybe_unused]] int scancode,
                                 int action, [[maybe_unused]] int mods)
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

    void GlfwWindow::MouseButtonCallback(GLFWwindow* window, int button,
                                         int action,
                                         [[maybe_unused]] int mods)
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

    void GlfwWindow::CursorPositionCallback(GLFWwindow* window, double xPos,
                                            double yPos)
    {
        auto const& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        Events::MouseMovedEvent event(xPos, yPos);
        data.eventCallback(event);
    }

    void GlfwWindow::ScrollCallback(GLFWwindow* window, double xOffset,
                                    double yOffset)
    {
        auto const& data = *static_cast<WindowData*>(
            glfwGetWindowUserPointer(window));

        Events::MouseScrolledEvent event(xOffset, yOffset);
        data.eventCallback(event);
    }
}