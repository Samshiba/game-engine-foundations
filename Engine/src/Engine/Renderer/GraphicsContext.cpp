//
// Created by genin on 07/02/2026.
//

#include <Engine/Renderer/RendererAPI.hpp>
#include <Engine/Platform/OpenGL/OpenGlContext.hpp>

#include "Commons.hpp"
#include "GraphicsContext.hpp"

namespace GEF::Renderer
{
    std::unique_ptr<GraphicsContext> GraphicsContext::Create(void* windowHandle,
        RendererBackend backend)
    {
        switch (backend)
        {
        case RendererBackend::OpenGL:
            GEF_ENGINE_DEBUG("Creating OpenGL context");
            return std::make_unique<Platform::OpenGLContext>(
                static_cast<GLFWwindow*>(windowHandle));

        case RendererBackend::Vulkan:
            GEF_ENGINE_ERROR("Vulkan not supported");
            return nullptr;

        case RendererBackend::DirectX11:
            GEF_ENGINE_ERROR("DX11 not supported");
            return nullptr;

        case RendererBackend::DirectX12:
            GEF_ENGINE_ERROR("DX12 not supported");
            return nullptr;

        case RendererBackend::Metal:
            GEF_ENGINE_ERROR("Metal not supported");
            return nullptr;

        case RendererBackend::None:
            GEF_ENGINE_ERROR("No Renderer Backend selected");
            return nullptr;
        }

        GEF_ENGINE_ERROR("Unknown Renderer Backend");
        return nullptr;
    }
}

