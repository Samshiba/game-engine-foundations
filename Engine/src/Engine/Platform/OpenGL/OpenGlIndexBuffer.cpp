//
// Created by genin on 09/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGlIndexBuffer.cpp
//

#include <glad/glad.h>

#include "OpenGlIndexBuffer.hpp"

namespace GEF::Platform
{
    OpenGlIndexBuffer::~OpenGlIndexBuffer()
    {
        glDeleteBuffers(1, &id_);
    }

    OpenGlIndexBuffer::OpenGlIndexBuffer(uint32_t* indices, uint32_t count)
        : count_(count)
    {
        glCreateBuffers(1, &id_);
        glNamedBufferData(id_, count * sizeof(uint32_t), indices,
                          GL_STATIC_DRAW);
    }

    void OpenGlIndexBuffer::Bind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
    }

    void OpenGlIndexBuffer::Unbind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    uint32_t OpenGlIndexBuffer::GetCount() const
    {
        return count_;
    }
}