//
// Created by genin on 26/01/2026.
//

#include <Engine/Core/Application.hpp>

#include "Core.hpp"

namespace GEF
{
    Application::Application([[maybe_unused]] int argc,
                             [[maybe_unused]] char** argv)
    {
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
            GEF_ENGINE_DEBUG("Running...");
            // DO
            is_running_ = false;
        }
        GEF_ENGINE_WARN("Application stopped");
    }
}
