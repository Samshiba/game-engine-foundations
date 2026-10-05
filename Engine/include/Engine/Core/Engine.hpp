//
// Created by genin on 26/01/2026.
// Path: Engine/include/Engine/Core/Engine.hpp
//

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include <Engine/Core/Input.hpp>
#include <Engine/Core/Window.hpp>
#include <Engine/Event/EventBus.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/GraphicsDevice.hpp>

namespace GEF
{
    struct EngineSpecification
    {
        std::string title = "GEF Engine";
        uint32_t width = 1280;
        uint32_t height = 720;
        Window::WindowMode windowMode = Window::WindowMode::WINDOWED;
        Window::CursorMode cursorMode = Window::CursorMode::NORMAL;
        Renderer::RendererBackend backend = Renderer::RendererBackend::OpenGL;
        Window::WindowInitialPosition position =
            Window::WindowInitialPosition::CENTER_PRIMARY_SCREEN;
        uint32_t flags = Window::FLAGS_VISIBLE;
    };

    // What the game is allowed to use.
    struct EngineContext
    {
        Window& window;
        Input& input;
        Renderer::GraphicsDevice& device;
        Events::EventBus& events;
    };

    // Any type with these member functions can be run by the Engine.
    // OnInit(EngineContext&) and OnShutdown(EngineContext&) are optional.
    template <typename T>
    concept Game = requires(T game, EngineContext& ctx, float dt,
                            Renderer::CommandList& cmd)
    {
        game.OnUpdate(ctx, dt);
        game.OnRender(ctx, cmd);
    };

    class Engine
    {
    public:
        explicit Engine(EngineSpecification spec);
        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        template <Game G>
        int Run(G& game);

    private:
        void BeginFrame();
        void EndFrame();

        void OnEvent(Events::Event& e);

    private:
        EngineSpecification spec_;
        // Declaration order is construction order: the window creates the GL
        // context the device needs, and is destroyed after it
        std::unique_ptr<Window> window_;
        std::unique_ptr<Renderer::GraphicsDevice> graphics_device_;
        Input input_;
        Events::EventBus event_bus_;

        std::shared_ptr<Renderer::CommandList> frame_commands_;
        bool is_running_ = true;
    };

    template <Game G>
    int Engine::Run(G& game)
    {
        using Clock = std::chrono::steady_clock;

        EngineContext ctx{ *window_, input_, *graphics_device_, event_bus_ };

        if constexpr (requires { game.OnInit(ctx); })
            game.OnInit(ctx);

        auto previous = Clock::now();
        while (is_running_)
        {
            const auto now = Clock::now();
            const float dt = std::chrono::duration<float>(now - previous).
                count();
            previous = now;

            input_.Update(*window_);
            game.OnUpdate(ctx, dt);

            BeginFrame();
            game.OnRender(ctx, *frame_commands_);
            EndFrame();
        }

        if constexpr (requires { game.OnShutdown(ctx); })
            game.OnShutdown(ctx);

        return 0;
    }
}
