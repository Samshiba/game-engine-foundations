//
// Created by genin on 16/02/2026.
// Path: Engine/src/Engine/Core/window.cpp
//

#include <Engine/Core/Window.hpp>
#include <Engine/Core/Application.hpp>
#include <Engine/Platform/Windows/WindowsWindow.hpp>

namespace GEF
{
    Window* Window::Create(const ApplicationSpecification& spec)
    {
        // TODO IMPLEMENT WindowInitialPosition
        auto props = WindowProps(spec.title, spec.width, spec.height, spec.mode,
                                 spec.flags);

        return new Platform::WindowsWindow(props, spec.backend);
    }
}