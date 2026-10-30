//
// Created by genin on 08/10/2026.
// Path: Engine/include/Engine/UI/ImGuiBackend.hpp
//

#pragma once

#include <memory>

#include <Engine/Core/Window.hpp>

namespace GEF::UI
{
    class ImGuiBackend
    {
    public:
        virtual ~ImGuiBackend() = default;

        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        static std::unique_ptr<ImGuiBackend>
        Create(Window& window, Renderer::RendererBackend backend);
    };
} // namespace GEF::UI