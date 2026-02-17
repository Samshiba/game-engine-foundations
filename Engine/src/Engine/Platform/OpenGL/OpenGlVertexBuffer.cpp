//
// Created by genin on 09/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGlVertexBuffer.cpp
//

#include <glad/glad.h>

#include "OpenGlVertexBuffer.hpp"

namespace GEF::Platform
{
    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        glDeleteBuffers(1, &id_);
    }

    OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
    {
        glCreateBuffers(1, &id_);
        glNamedBufferData(id_, size, vertices, GL_STATIC_DRAW);
    }

    void OpenGLVertexBuffer::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, id_);
    }

    void OpenGLVertexBuffer::Unbind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void OpenGLVertexBuffer::SetLayout(const Renderer::BufferLayout& layout)
    {
        layout_ = layout;
    }

    const Renderer::BufferLayout& OpenGLVertexBuffer::GetLayout() const
    {
        return layout_;
    }
}