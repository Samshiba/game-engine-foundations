//
// Created by genin on 26/01/2026.
//

#pragma once

#include "Window.hpp"
#include <Engine/Event/EventBus.hpp>

namespace GEF
{
    class Application
    {
    public:
        Application(int argc, char** argv);
        virtual ~Application() = 0;

        void Run();

        [[nodiscard]] static Application& Get();
        [[nodiscard]] Window& GetWindow() const;

        void OnEvent(Events::Event& e);

    private:
        bool is_running_ = true;
        std::unique_ptr<Window> window_;
        Events::EventBus event_bus_;
        static Application* instance_;
    };

    Application* CreateApplication(int argc, char** argv);
}
