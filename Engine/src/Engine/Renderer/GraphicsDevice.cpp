//
// Created by genin on 16/02/2026.
// Path: Engine/src/Engine/Renderer/GraphicsDevice.cpp
//

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/RendererAPI.hpp>

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>

namespace GEF::Renderer
{
    // Static member init
    GraphicsDevice* GraphicsDevice::instance_ = nullptr;

    std::unique_ptr<GraphicsDevice> GraphicsDevice::Create(
        RendererBackend backend)
    {
        switch (backend)
        {
        case RendererBackend::OpenGL:
            return std::make_unique<Platform::OpenGLGraphicsDevice>();
        case RendererBackend::Vulkan:
            GEF_ENGINE_ERROR("Vulkan backend is not supported yet");
            return nullptr;
        case RendererBackend::DirectX11:
            GEF_ENGINE_ERROR("DirectX11 backend is not supported yet");
            return nullptr;
        case RendererBackend::DirectX12:
            GEF_ENGINE_ERROR("DirectX12 backend is not supported yet");
            return nullptr;
        case RendererBackend::Metal:
            GEF_ENGINE_ERROR("Metal backend is not supported yet");
            return nullptr;
        default:
            GEF_ENGINE_ERROR("Unknown RendererBackend");
            return nullptr;
        }
    }
}