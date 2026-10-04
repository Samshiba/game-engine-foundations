//
// Created by genin on 26/01/2026.
// Path: Engine/src/Engine/Core/Engine.cpp
//

#include <Engine/Core/Engine.hpp>
#include <Engine/Event/ApplicationEvent.hpp>

#include "Commons.hpp"

#include <glad/glad.h>

namespace GEF
{
    Engine::Engine(EngineSpecification spec)
        : spec_(std::move(spec)),
          window_(Window::Create(spec_))
    {
        auto device = Renderer::GraphicsDevice::Create(spec_.backend);
        GEF_CORE_ASSERT(device, "Failed to create graphics device");
        graphics_device_ = std::move(device);
        window_->SetEventCallback([this](Events::Event& e) {
            OnEvent(e);
        });
        GEF_ENGINE_INFO("Engine created");
    }

    Engine::~Engine()
    {
        GEF_ENGINE_INFO("Engine destroyed");
    }

    void Engine::BeginFrame()
    {
        frame_commands_ = graphics_device_->BeginCommandList();
        frame_commands_->SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        frame_commands_->Clear();
    }

    void Engine::EndFrame()
    {
        graphics_device_->SubmitCommandList(frame_commands_);
        window_->OnUpdate(); // swap buffers + poll events
    }

    void Engine::OnEvent(Events::Event& e)
    {
        if (e.GetEventType() == Events::WindowCloseEvent::GetStaticEventType())
        {
            is_running_ = false;
        }
        else if (e.GetEventType() ==
            Events::WindowResizeEvent::GetStaticEventType())
        {
            // TODO(GEF-38): goes through the CommandList once it records
            const auto& resize = static_cast<Events::WindowResizeEvent&>(e);
            glViewport(0, 0, static_cast<GLsizei>(resize.GetWidth()),
                       static_cast<GLsizei>(resize.GetHeight()));
        }

        event_bus_.TriggerEvent(e);
    }
}
