//
// Created by genin on 16/02/2026.
// Path: Engine/src/Engine/Core/Window.cpp
//

#include <Engine/Core/Window.hpp>
#include <Engine/Core/Application.hpp>
#include <Engine/Platform/Glfw/GlfwWindow.hpp>

namespace GEF
{
    Window* Window::Create(const ApplicationSpecification& spec)
    {
        // TODO IMPLEMENT WindowInitialPosition
        const WindowProps props{ .title = spec.title,
                                 .width = spec.width,
                                 .height = spec.height,
                                 .mode = spec.mode,
                                 .flags = spec.flags };

        return new Platform::GlfwWindow(props, spec.backend);
    }
}