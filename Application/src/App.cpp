//
// Created by genin on 26/01/2026.
//

#include "App.hpp"

GEF::Application* GEF::CreateApplication(int argc, char** argv)
{
    return new App(argc, argv);
}
