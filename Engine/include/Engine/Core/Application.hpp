//
// Created by genin on 26/01/2026.
// Path: Engine/include/Engine/Core/Application.hpp
//

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Window.hpp"
#include <Engine/Event/EventBus.hpp>
#include <Engine/Renderer/GraphicsDevice.hpp>

namespace GEF
{
    struct ApplicationSpecification
    {
        std::string title = "GEF Engine";
        uint32_t width = 1280;
        uint32_t height = 720;
        Window::WindowMode mode = Window::WindowMode::WINDOWED;
        Renderer::RendererBackend backend = Renderer::RendererBackend::OpenGL;
        Window::WindowInitialPosition position =
            Window::WindowInitialPosition::CENTER_PRIMARY_SCREEN;
        uint32_t flags = Window::FLAGS_VISIBLE;
    };

    class Application
    {
    public:
        Application(const ApplicationSpecification& spec);
        virtual ~Application() = 0;

        void Run();

        [[nodiscard]] static Application& Get();
        [[nodiscard]] Window& GetWindow() const;
        [[nodiscard]] ApplicationSpecification GetSpecification() const;

        void OnEvent(Events::Event& e);

    private:
        const ApplicationSpecification& spec_;
        bool is_running_ = true;
        std::unique_ptr<Window> window_;
        std::unique_ptr<Renderer::GraphicsDevice> graphics_device_;
        Events::EventBus event_bus_;
        static Application* instance_;
    };

    Application* CreateApplication(ApplicationSpecification& spec);
}
