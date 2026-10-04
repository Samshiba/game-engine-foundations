//
// Created by genin on 15/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.cpp
//

#include "OpenGLGraphicsDevice.hpp"
#include "OpenGLCommandList.hpp"

namespace GEF::Platform
{
    OpenGLGraphicsDevice::~OpenGLGraphicsDevice()
    {
        // Clean vertex arrays first, they reference buffers
        const auto vertexArrays = vertexArrays_.GetAliveHandles();
        if (!vertexArrays.empty())
        {
            GEF_ENGINE_WARN("{} vertex arrays leaked! Destroy them!",
                            vertexArrays.size());
            for (const auto& handle : vertexArrays)
                OpenGLGraphicsDevice::DestroyVertexArray(handle);
        }

        const auto buffers = buffers_.GetAliveHandles();
        if (!buffers.empty())
        {
            GEF_ENGINE_WARN("{} buffers leaked! Destroy them!", buffers.size());
            for (const auto& handle : buffers)
                OpenGLGraphicsDevice::DestroyBuffer(handle);
        }
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

    const GLVertexArray* OpenGLGraphicsDevice::GetVertexArray(
        Renderer::VertexArrayHandle handle) const
    {
        return vertexArrays_.IsValid(handle)
            ? vertexArrays_.Get(handle)
            : nullptr;
    }

    Renderer::VertexArrayHandle OpenGLGraphicsDevice::CreateVertexArray(
        const Renderer::VertexArrayDesc& desc)
    {
        GLuint vao = 0;
        glCreateVertexArrays(1, &vao);

        // Attribute = one shader input (layout(location = N)): format + offset.
        // Binding   = one buffer slot: which buffer, stride, per-vertex or
        //             per-instance. Each attribute reads from one binding.
        GLuint attrib = 0;
        for (GLuint b = 0; b < desc.vertexBuffers.size(); ++b)
        {
            const auto& binding = desc.vertexBuffers[b];
            const GLBuffer* buffer = GetBuffer(binding.buffer);
            GEF_CORE_ASSERT(buffer, "Invalid vertex buffer handle");

            glVertexArrayVertexBuffer(vao, b, buffer->id, 0,
                                      static_cast<GLsizei>(
                                          binding.layout.GetStride()));
            glVertexArrayBindingDivisor(vao, b, binding.instanceDivisor);

            const auto addAttribute = [&](GLint count, GLenum type,
                                          bool isInteger, bool normalized,
                                          size_t offset) {
                const auto relativeOffset = static_cast<GLuint>(offset);
                glEnableVertexArrayAttrib(vao, attrib);
                if (isInteger)
                    glVertexArrayAttribIFormat(vao, attrib, count, type,
                                               relativeOffset);
                else
                    glVertexArrayAttribFormat(vao, attrib, count, type,
                                              normalized ? GL_TRUE : GL_FALSE,
                                              relativeOffset);
                glVertexArrayAttribBinding(vao, attrib, b);
                ++attrib;
            };

            for (const Renderer::BufferElement& element : binding.layout)
            {
                const GLint count = element.GetComponentCount();
                switch (element.type)
                {
                    using enum Renderer::ShaderDataType;
                case Float:
                case Float2:
                case Float3:
                case Float4:
                    addAttribute(count, GL_FLOAT, false, element.normalized,
                                 element.offset);
                    break;
                case Int:
                case Int2:
                case Int3:
                case Int4:
                case Bool:
                    addAttribute(count, GL_INT, true, false, element.offset);
                    break;
                case Mat3:
                case Mat4:
                    for (GLint column = 0; column < count; ++column)
                        addAttribute(count, GL_FLOAT, false, element.normalized,
                                     element.offset + sizeof(float) * count *
                                     column);
                    break;
                default:
                    GEF_CORE_ASSERT(false, "Unsupported data type");
                }
            }
        }

        const GLBuffer* indexBuffer = GetBuffer(desc.indexBuffer);
        GEF_CORE_ASSERT(indexBuffer, "Invalid index buffer handle");
        glVertexArrayElementBuffer(vao, indexBuffer->id);

        return vertexArrays_.Insert(GLVertexArray{ vao });
    }

    void OpenGLGraphicsDevice::DestroyVertexArray(
        Renderer::VertexArrayHandle handle)
    {
        auto vertexArray = vertexArrays_.Get(handle);
        if (vertexArray)
        {
            glDeleteVertexArrays(1, &vertexArray->id);
            vertexArrays_.Remove(handle);
        }
    }

    std::shared_ptr<Renderer::CommandList> OpenGLGraphicsDevice::
    BeginCommandList()
    {
        return std::make_shared<OpenGLCommandList>(*this);
    }

    void OpenGLGraphicsDevice::SubmitCommandList(
        const std::shared_ptr<Renderer::CommandList>& commandList)
    {
        (void)commandList;
    }
}