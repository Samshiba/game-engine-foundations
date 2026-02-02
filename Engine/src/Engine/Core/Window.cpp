//
// Created by genin on 02/02/2026.
//

#include <Engine/Platform/Windows/WindowsWindow.hpp>

namespace GEF
{
    Window* Window::Create(const WindowProps& props)
    {
        // TODO REFACTO TO NOT HARDCODE WINDOWS WINDOW
        return new Platform::WindowsWindow(props);
    }
}