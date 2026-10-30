//
// Created by genin on 26/01/2026.
// Path: Engine/src/Engine/Core/Engine.cpp
//

#include <Engine/Core/Engine.hpp>

#include <Engine/Core/FileSystem.hpp>
#include <Engine/Event/ApplicationEvent.hpp>

#include "Commons.hpp"

namespace GEF
{
    Engine::Engine(EngineSpecification spec)
        : spec_(std::move(spec))
        , window_(Window::Create(spec_))
    {
        auto device = Renderer::GraphicsDevice::Create(spec_.backend);
        GEF_CORE_ASSERT(device, "Failed to create graphics device");
        graphics_device_ = std::move(device);
        window_->SetEventCallback([this](Events::Event& e) { OnEvent(e); });

        if (!std::filesystem::is_directory(FileSystem::AssetsDirectory()))
            GEF_ENGINE_ERROR("Assets directory not found: {}",
                             FileSystem::AssetsDirectory().string());

        auto imgui = UI::ImGuiBackend::Create(*window_, spec_.backend);
        GEF_CORE_ASSERT(imgui, "Failed to create ImGui backend");
        imgui_ = std::move(imgui);

        GEF_ENGINE_INFO("Engine created");
    }

    Engine::~Engine()
    {
        GEF_ENGINE_INFO("Engine destroyed");
    }

    void Engine::BeginFrame()
    {
        frame_commands_.Reset();
        frame_commands_.SetViewport(0, 0, spec_.width, spec_.height);
        frame_commands_.SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        frame_commands_.Clear();

        imgui_->BeginFrame();
    }

    void Engine::EndFrame()
    {
        graphics_device_->Submit(frame_commands_);
        imgui_->EndFrame();
        window_->OnUpdate(); // swap buffers + poll events
    }

    void Engine::OnEvent(Events::Event& e)
    {
        if (e.GetEventType() == Events::WindowCloseEvent::GetStaticEventType())
        {
            is_running_ = false;
        }
        else if (e.GetEventType()
                 == Events::WindowResizeEvent::GetStaticEventType())
        {
            const auto& resize = static_cast<Events::WindowResizeEvent&>(e);
            spec_.width = resize.GetWidth();
            spec_.height = resize.GetHeight();
        }

        event_bus_.TriggerEvent(e);
    }
} // namespace GEF
