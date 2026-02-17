//
// Created by genin on 15/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.cpp
//

#include "OpenGLGraphicsDevice.hpp"

#include "OpenGLCommandList.hpp"

namespace GEF::Platform
{
    OpenGLGraphicsDevice::OpenGLGraphicsDevice()
    {
        instance_ = this;
    }

    std::shared_ptr<Renderer::IndexBuffer>
    OpenGLGraphicsDevice::CreateIndexBuffer(
        uint32_t* indices, uint32_t count)
    {
        return std::make_shared<OpenGlIndexBuffer>(indices, count);
    }

    std::shared_ptr<Renderer::VertexBuffer> OpenGLGraphicsDevice::
    CreateVertexBuffer(float* vertices, uint32_t size)
    {
        return std::make_shared<OpenGLVertexBuffer>(vertices, size);
    }

    std::shared_ptr<Renderer::VertexArray>
    OpenGLGraphicsDevice::CreateVertexArray()
    {
        return std::make_shared<OpenGlVertexArray>();
    }

    std::shared_ptr<Renderer::CommandList> OpenGLGraphicsDevice::
    BeginCommandList()
    {
        return std::make_shared<OpenGLCommandList>();
    }

    void OpenGLGraphicsDevice::SubmitCommandList(
        const std::shared_ptr<Renderer::CommandList>& commandList)
    {
        (void)commandList;
    }
}