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
    namespace
    {
        // TODO(GEF-32): replaced by GraphicsDevice::CreateShader
        constexpr const char* DebugVertexShader = R"(
            #version 450 core
            layout (location = 0) in vec3 a_Position;
            layout (location = 1) in vec3 a_Color;
            out vec3 v_Color;
            void main()
            {
                v_Color = a_Color;
                gl_Position = vec4(a_Position, 1.0);
            })";

        constexpr const char* DebugFragmentShader = R"(
            #version 450 core
            in vec3 v_Color;
            out vec4 o_Color;
            void main()
            {
                o_Color = vec4(v_Color, 1.0);
            })";

        GLuint CompileStage(GLenum stage, const char* source)
        {
            const GLuint shader = glCreateShader(stage);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            GLint success = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                GLchar log[512];
                glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
                GEF_ENGINE_ERROR("Shader compilation failed: {}", log);
            }
            return shader;
        }

        GLuint CreateDebugProgram()
        {
            const GLuint vs = CompileStage(GL_VERTEX_SHADER, DebugVertexShader);
            const GLuint fs = CompileStage(GL_FRAGMENT_SHADER,
                                           DebugFragmentShader);

            const GLuint program = glCreateProgram();
            glAttachShader(program, vs);
            glAttachShader(program, fs);
            glLinkProgram(program);

            GLint success = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &success);
            if (!success)
            {
                GLchar log[512];
                glGetProgramInfoLog(program, sizeof(log), nullptr, log);
                GEF_ENGINE_ERROR("Shader link failed: {}", log);
            }

            glDeleteShader(vs);
            glDeleteShader(fs);
            return program;
        }
    }

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
        debug_shader_ = CreateDebugProgram();
        GEF_ENGINE_INFO("Engine created");
    }

    Engine::~Engine()
    {
        glDeleteProgram(debug_shader_);
        GEF_ENGINE_INFO("Engine destroyed");
    }

    void Engine::BeginFrame()
    {
        frame_commands_ = graphics_device_->BeginCommandList();
        frame_commands_->SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        frame_commands_->Clear();

        glUseProgram(debug_shader_); // TODO(GEF-32): cmd.BindShader by the game
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
