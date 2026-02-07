//
// Created by genin on 26/01/2026.
//

#include <Engine/Core/Application.hpp>
#include <Engine/Event/ApplicationEvent.hpp>

#include "Commons.hpp"

namespace GEF
{
    Application::Application([[maybe_unused]] int argc,
                             [[maybe_unused]] char** argv)
    {
        window_ = std::unique_ptr<Window>(Window::Create());

        window_->SetEventCallback([this](Events::Event& e) {
            this->OnEvent(e);
        });
        GEF_ENGINE_INFO("Application created");
    }

    Application::~Application()
    {
        GEF_ENGINE_INFO("Application destroyed");
    }

    void Application::Run()
    {
        while (is_running_)
        {
            // DO
            window_->OnUpdate();
        }
        GEF_ENGINE_WARN("Application stopped");
    }

    void Application::OnEvent(Events::Event& e)
    {
        GEF_ENGINE_INFO(e.ToString());

        if (e.GetEventType() == Events::WindowCloseEvent::GetStaticEventType())
        {
            is_running_ = false;
        }

        event_bus_.TriggerEvent(e);
    }
}
