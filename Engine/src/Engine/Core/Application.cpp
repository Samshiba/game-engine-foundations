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
        std::cout << "Application created" << std::endl;
    }

    Application::~Application()
    {
        std::cout << "Application destroyed" << std::endl;
    }

    void Application::Run()
    {
        while (is_running_)
        {
            std::cout << "Running..." << std::endl;
            // DO
            is_running_ = false;
        }
        std::cout << "Application stopped" << std::endl;
    }
}
