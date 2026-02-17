//
// Created by genin on 26/01/2026.
// Path: Application/src/App.cpp
//

#include "App.hpp"

GEF::Application* GEF::CreateApplication(ApplicationSpecification& spec)
{
    GEF_INFO("Starting Game");
    spec.title = "My Super Awesome Game!";
    spec.mode = Window::WindowMode::WINDOWED;
    return new App(spec);
}
