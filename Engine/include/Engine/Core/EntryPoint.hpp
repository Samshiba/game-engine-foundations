//
// Created by genin on 27/01/2026.
// Path: Engine/include/Engine/Core/EntryPoint.hpp
//

#pragma once

#include "Application.hpp"
#include "Log.hpp"

extern GEF::Application* GEF::CreateApplication(ApplicationSpecification& spec);

int main()
{
    GEF::Log::Init();

    auto spec = GEF::ApplicationSpecification();
    auto app = GEF::CreateApplication(spec);
    app->Run();

    delete app;
    return 0;
}