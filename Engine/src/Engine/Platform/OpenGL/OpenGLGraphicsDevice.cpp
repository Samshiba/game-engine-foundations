//
// Created by genin on 15/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.cpp
//

#include "OpenGLGraphicsDevice.hpp"
#include "OpenGLCommandList.hpp"
#include "OpenGlIndexBuffer.hpp"
#include "OpenGlVertexBuffer.hpp"

namespace GEF::Platform
{
    OpenGLGraphicsDevice::~OpenGLGraphicsDevice()
    {
        auto handles = buffers_.GetAliveHandles();

        if (handles.empty())
            return;

        GEF_ENGINE_WARN("{} buffers leaked ! Destroy them !", handles.size());
        for (const auto& handle : handles)
        {
            auto buffer = buffers_.Get(handle);
            if (buffer)
            {
                glDeleteBuffers(1, &buffer->id);
                buffers_.Remove(handle);
            }
        }
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

    Renderer::BufferHandle OpenGLGraphicsDevice::CreateBuffer(
        const Renderer::BufferDesc& desc)
    {
        GLuint id = 0;
        glCreateBuffers(1, &id);
        glNamedBufferData(id, desc.size, desc.data, GL_STATIC_DRAW);
        return buffers_.Insert(GLBuffer{ id, desc.size, desc.type });
    }

    void OpenGLGraphicsDevice::DestroyBuffer(Renderer::BufferHandle handle)
    {
        auto buffer = buffers_.Get(handle);
        if (buffer)
        {
            glDeleteBuffers(1, &buffer->id);
            buffers_.Remove(handle);
        }
    }

    const GLBuffer* OpenGLGraphicsDevice::GetBuffer(
        Renderer::BufferHandle handle) const
    {
        return buffers_.IsValid(handle) ? buffers_.Get(handle) : nullptr;
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