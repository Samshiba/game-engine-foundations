//
// Created by genin on 08/10/2026.
// Path: Engine/src/Engine/UI/ImGuiBackend.cpp
//

#include <Engine/UI/ImGuiBackend.hpp>
#include <Engine/Platform/ImGui/GlfwOpenGLImGuiBackend.hpp>

std::unique_ptr<GEF::UI::ImGuiBackend> GEF::UI::ImGuiBackend::Create(
    Window& window, Renderer::RendererBackend backend)
{
    switch (backend)
    {
    case Renderer::RendererBackend::OpenGL:
        return std::make_unique<Platform::GlfwOpenGLImGuiBackend>(window);
    case Renderer::RendererBackend::Vulkan:
        GEF_ENGINE_ERROR("Vulkan backend is not supported yet");
        return nullptr;
    case Renderer::RendererBackend::DirectX11:
        GEF_ENGINE_ERROR("DirectX11 backend is not supported yet");
        return nullptr;
    case Renderer::RendererBackend::DirectX12:
        GEF_ENGINE_ERROR("DirectX12 backend is not supported yet");
        return nullptr;
    case Renderer::RendererBackend::Metal:
        GEF_ENGINE_ERROR("Metal backend is not supported yet");
        return nullptr;
    default:
        GEF_ENGINE_ERROR("Unknown RendererBackend");
        return nullptr;
    }
}
