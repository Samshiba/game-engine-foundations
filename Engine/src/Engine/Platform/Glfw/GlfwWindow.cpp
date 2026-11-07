//
// Created by genin on 02/02/2026.
// Path: Engine/src/Engine/Platform/Glfw/GlfwWindow.cpp
//

#include "GlfwWindow.hpp"

#include <Engine/Event/ApplicationEvent.hpp>
#include <Engine/Platform/OpenGL/OpenGlContext.hpp>

#include "Commons.hpp"

namespace GEF::Platform
{
    namespace
    {
        // GLFW is initialized with the first window and terminated with the
        // last one, so windows can be created and destroyed in any order
        uint32_t s_WindowCount = 0;
    } // namespace

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

    namespace
    {
        GLFWmonitor* GetMonitorByIndex(uint8_t index)
        {
            int count;
            GLFWmonitor** monitors =
                glfwGetMonitors(&count); // first is always main monitor
            if (index < static_cast<uint8_t>(count))
            {
                return monitors[index];
            }
            return glfwGetPrimaryMonitor();
        }

        void CenterWindowOnMonitor(GLFWwindow* window, GLFWmonitor* monitor,
                                   int width, int height)
        {
            if (!monitor)
                return;

            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (!mode)
                return;

            int monitorX;
            int monitorY;
            glfwGetMonitorPos(monitor, &monitorX, &monitorY);

            int posX = monitorX + (mode->width - width) / 2;
            int posY = monitorY + (mode->height - height) / 2;

            glfwSetWindowPos(window, posX, posY);
        }
    } // namespace

    GLFWwindow* GlfwWindow::CreateWindow(const WindowProps& props)
    {
        GLFWmonitor* monitor = nullptr;

        if (props.windowPositionConfig.type
            == WindowInitialPosition::CENTER_OTHER_SCREEN)
        {
            monitor = GetMonitorByIndex(1);
        }
        else if (props.windowPositionConfig.type
                 == WindowInitialPosition::CENTER_MONITOR_INDEX)
        {
            monitor =
                GetMonitorByIndex(props.windowPositionConfig.monitorIndex);
        }
        else
        {
            monitor = glfwGetPrimaryMonitor();
        }

        if (props.windowMode == WindowMode::MAXIMIZED)
        {
            glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        }
        if (props.windowMode == WindowMode::MINIMIZED
            || props.windowMode == WindowMode::WINDOWED)
        {
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        }

        GLFWmonitor* windowMonitorArg = nullptr;
        if (props.windowMode == WindowMode::FULLSCREEN
            || props.windowMode == WindowMode::EXCLUSIVE)
        {
            windowMonitorArg = monitor;

            if (props.windowMode == WindowMode::FULLSCREEN)
            {
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwWindowHint(GLFW_RED_BITS, mode->redBits);
                glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
                glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
                glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            }
        }

        GLFWwindow* window = glfwCreateWindow(
            static_cast<int>(props.width), static_cast<int>(props.height),
            props.title.c_str(), windowMonitorArg, nullptr);

        if (!window)
        {
            GEF_ENGINE_ERROR("Failed to create GLFW window");
            if (s_WindowCount == 0)
                glfwTerminate();
            return nullptr;
        }

        if (props.windowMode == WindowMode::WINDOWED)
        {
            switch (props.windowPositionConfig.type)
            {
            case WindowInitialPosition::ABSOLUTE:
                glfwSetWindowPos(window, props.windowPositionConfig.absoluteX,
                                 props.windowPositionConfig.absoluteY);
                break;

            case WindowInitialPosition::CENTER_PRIMARY_SCREEN:
            case WindowInitialPosition::CENTER_OTHER_SCREEN:
            case WindowInitialPosition::CENTER_MONITOR_INDEX:
                CenterWindowOnMonitor(window, monitor,
                                      static_cast<int>(props.width),
                                      static_cast<int>(props.height));
                break;
            }
            glfwShowWindow(window);
        }
        else if (props.windowMode == WindowMode::MINIMIZED)
        {
            glfwIconifyWindow(window);
            glfwShowWindow(window);
        }

        return window;
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

        if (s_WindowCount == 0)
        {
            glfwSetErrorCallback(ErrorCallback);
            [[maybe_unused]] const int success = glfwInit();
            GEF_CORE_ASSERT(success, "Failed to initialize GLFW");
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

        glfwWindowHint(GLFW_RESIZABLE,
                       (props.flags & FLAGS_RESIZE_DISABLED) ? GLFW_FALSE
                                                             : GLFW_TRUE);

        auto window = CreateWindow(props);
        if (!window)
        {
            GEF_ENGINE_ERROR("Failed to create GLFW window");
            if (s_WindowCount == 0)
                glfwTerminate();
            return;
        }
        window_ = window;

        context_ = Renderer::GraphicsContext::Create(window_, backend);
        if (!context_)
        {
            GEF_ENGINE_ERROR("Failed to create Graphics Context");
            glfwDestroyWindow(window_);
            window_ = nullptr;
            if (s_WindowCount == 0)
                glfwTerminate();
            return;
        }
        context_->Init();
        ++s_WindowCount;

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
        if (!window_)
            return;

        GEF_ENGINE_INFO("Destroying window");
        glfwDestroyWindow(window_);
        window_ = nullptr;

        if (--s_WindowCount == 0)
            glfwTerminate();
    }

    void GlfwWindow::Close()
    {
        // Same path as the user clicking the close button
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
        Events::WindowCloseEvent event;
        if (windowData_.eventCallback)
            windowData_.eventCallback(event);
    }

    void GlfwWindow::ErrorCallback([[maybe_unused]] int error,
                                   [[maybe_unused]] const char* description)
    {
        GEF_ENGINE_ERROR("GLFW Error : ({}) : {}", error, description);
    }

    void GlfwWindow::WindowResizeCallback(GLFWwindow* window, int width,
                                          int height)
    {
        auto& data =
            *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        data.width = width;
        data.height = height;

        Events::WindowResizeEvent event(width, height);
        data.eventCallback(event);
    }

    void GlfwWindow::WindowCloseCallback(GLFWwindow* window)
    {
        auto const& data =
            *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        Events::WindowCloseEvent event;
        data.eventCallback(event);
    }

    void GlfwWindow::KeyCallback(GLFWwindow* window, int key,
                                 [[maybe_unused]] int scancode, int action,
                                 [[maybe_unused]] int mods)
    {
        (void)scancode;
        (void)mods;

        auto const& data =
            *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

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
                                         int action, [[maybe_unused]] int mods)
    {
        (void)mods;

        auto const& data =
            *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

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
        auto const& data =
            *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        Events::MouseMovedEvent event(xPos, yPos);
        data.eventCallback(event);
    }

    void GlfwWindow::ScrollCallback(GLFWwindow* window, double xOffset,
                                    double yOffset)
    {
        auto const& data =
            *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

        Events::MouseScrolledEvent event(xOffset, yOffset);
        data.eventCallback(event);
    }
} // namespace GEF::Platform