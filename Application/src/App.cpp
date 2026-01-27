//
// Created by genin on 26/01/2026.
//

#include "App.hpp"

GEF::Application* GEF::CreateApplication(int argc, char** argv)
{
    GEF_INFO("Starting Game");
    return new App(argc, argv);
}
