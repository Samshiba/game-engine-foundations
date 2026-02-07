//
// Created by genin on 02/02/2026.
//

#pragma once

#include <Engine/Renderer/RendererAPI.hpp>

#include "Commons.hpp"

namespace GEF::Renderer
{
    class GraphicsContext
    {
    public:
        virtual ~GraphicsContext() = default;

        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;

        static std::unique_ptr<GraphicsContext> Create(
            void* windowHandle, RendererBackend backend);
    };
}