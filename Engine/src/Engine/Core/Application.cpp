//
// Created by genin on 26/01/2026.
// Path: Engine/src/Engine/Core/Application.cpp
//

#include <Engine/Core/Application.hpp>
#include <Engine/Event/ApplicationEvent.hpp>
#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/Buffer.hpp>

#include "Commons.hpp"
#include "Input.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

const GLchar* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 color;\n"
    "out vec3 ourColor;\n"
    "void main()\n"
    "{\n"
    "ourColor = color;\n"
    "gl_Position = vec4(position, 1.0);\n"
    "}\0";

const GLchar* fragmentShaderSource = "#version 330 core\n"
    "in vec3 ourColor;\n"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    "color = vec4(ourColor, 1.0);\n"
    "}\n\0";

namespace GEF
{
    // Static member init
    Application* Application::instance_ = nullptr;


    Application::Application(const ApplicationSpecification& spec)
        : spec_(spec)
    {
        window_ = std::unique_ptr<Window>(Window::Create(spec));

        graphics_device_ = Renderer::GraphicsDevice::Create(spec.backend);

        window_->SetEventCallback([this](Events::Event& e) {
            this->OnEvent(e);
        });
        instance_ = this;
        GEF_ENGINE_INFO("Application created");
    }

    Application::~Application()
    {
        GEF_ENGINE_INFO("Application destroyed");
    }

    void Application::Run()
    {
        // Build and compile our shader program
        // Vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // Check for compile time errors
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" <<
                infoLog << std::endl;
        }
        // Fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        // Check for compile time errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" <<
                infoLog << std::endl;
        }
        // Link shaders
        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        // Check for linking errors
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog
                << std::endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Set up vertex data (and buffer(s)) and attribute pointers
        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // Bas Gauche (Rouge)
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // Bas Droite (Vert)
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f // Haut Centre (Bleu)
        };
        uint32_t indices[] = { 0, 1, 2 };

        auto vertexBuffer = graphics_device_->CreateVertexBuffer(
            vertices, sizeof(vertices));
        vertexBuffer->SetLayout(
        { { Renderer::ShaderDataType::Float3, "position" },
          { Renderer::ShaderDataType::Float3, "color" } });
        auto vertexArray = graphics_device_->CreateVertexArray();
        auto layout = Renderer::BufferLayout();
        vertexArray->AddVertexBuffer(vertexBuffer);

        auto indexBuffer = graphics_device_->CreateIndexBuffer(indices, 3);
        vertexArray->SetIndexBuffer(indexBuffer);

        while (is_running_)
        {
            Input::Update();

            // Render
            auto cmd = graphics_device_->BeginCommandList();
            cmd->SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            cmd->Clear();

            glUseProgram(shaderProgram);

            cmd->BindVertexArray(vertexArray);
            cmd->DrawIndexed(vertexArray->GetIndexBuffer()->GetCount());

            graphics_device_->SubmitCommandList(cmd);

            window_->OnUpdate();
        }
        GEF_ENGINE_WARN("Application stopped");
    }

    void Application::OnEvent(Events::Event& e)
    {
        GEF_ENGINE_INFO(e.ToString());

        if (e.GetEventType() == Events::WindowCloseEvent::GetStaticEventType())
        {
            is_running_ = false;
        }

        event_bus_.TriggerEvent(e);
    }

    Application& Application::Get()
    {
        return *instance_;
    }

    Window& Application::GetWindow() const
    {
        return *window_;
    }

    ApplicationSpecification Application::GetSpecification() const
    {
        return spec_;
    }
}