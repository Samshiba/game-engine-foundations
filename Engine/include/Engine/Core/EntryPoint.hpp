//
// Created by genin on 27/01/2026.
// Path: Engine/include/Engine/Core/EntryPoint.hpp
//

#pragma once

#include "Application.hpp"
#include "Log.hpp"

#ifdef GEF_PLATFORM_WINDOWS

extern GEF::Application* GEF::CreateApplication(ApplicationSpecification& spec);

int main(int argc, char** argv)
{
    GEF::Log::Init();

    auto spec = GEF::ApplicationSpecification();
    auto app = GEF::CreateApplication(spec);
    app->Run();

    delete app;
    return 0;
}

#endif