//
// Created by genin on 08/10/2026.
// Path: Engine/src/Engine/Platform/ImGui/GlfwOpenGLImGuiBackend.hpp
//

#pragma once

#include <Engine/UI/ImGuiBackend.hpp>
#include <Engine/Core/Window.hpp>

namespace GEF::Platform
{
    class GlfwOpenGLImGuiBackend : public UI::ImGuiBackend
    {
    public:
        explicit GlfwOpenGLImGuiBackend(Window& window);
        ~GlfwOpenGLImGuiBackend() override;

        void BeginFrame() override;
        void EndFrame() override;
    };
}