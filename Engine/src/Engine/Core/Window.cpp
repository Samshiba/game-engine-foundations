//
// Created by genin on 16/02/2026.
// Path: Engine/src/Engine/Core/Window.cpp
//

#include <Engine/Core/Window.hpp>
#include <Engine/Core/Engine.hpp>
#include <Engine/Platform/Glfw/GlfwWindow.hpp>

namespace GEF
{
    std::unique_ptr<Window> Window::Create(const EngineSpecification& spec)
    {
        // TODO IMPLEMENT WindowInitialPosition
        const WindowProps props{ .title = spec.title,
                                 .width = spec.width,
                                 .height = spec.height,
                                 .windowMode = spec.windowMode,
                                 .cursorMode = spec.cursorMode,
                                 .flags = spec.flags };

        return std::make_unique<Platform::GlfwWindow>(props, spec.backend);
    }
}