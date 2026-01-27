//
// Created by genin on 27/01/2026.
//

#pragma once

#include "Application.hpp"

#ifdef GEF_PLATFORM_WINDOWS

extern GEF::Application* GEF::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
    // LOG

    auto app = GEF::CreateApplication(argc, argv);
    app->Run();

    delete app;
    return 0;
}

#endif