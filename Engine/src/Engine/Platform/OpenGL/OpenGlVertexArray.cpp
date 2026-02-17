//
// Created by genin on 09/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGlVertexArray.cpp
//

#include <glad/glad.h>

#include "OpenGlVertexArray.hpp"

namespace GEF::Platform
{
    OpenGlVertexArray::~OpenGlVertexArray()
    {
        glDeleteVertexArrays(1, &id_);
    }

    OpenGlVertexArray::OpenGlVertexArray()
    {
        glCreateVertexArrays(1, &id_);
        glBindVertexArray(id_);
    }

    void OpenGlVertexArray::Bind() const
    {
        glBindVertexArray(id_);
    }

    void OpenGlVertexArray::Unbind() const
    {
        glBindVertexArray(0);
    }

    void OpenGlVertexArray::AddVertexBuffer(
        const std::shared_ptr<Renderer::VertexBuffer>& vertexBuffer)
    {
        glBindVertexArray(id_);
        vertexBuffer->Bind();

        const auto& layout = vertexBuffer->GetLayout();
        for (const auto& element : layout)
        {
            switch (element.type)
            {
            case Renderer::ShaderDataType::Float:
            case Renderer::ShaderDataType::Float2:
            case Renderer::ShaderDataType::Float3:
            case Renderer::ShaderDataType::Float4: {
                glEnableVertexAttribArray(vertexBufferCount_);
                glVertexAttribPointer(vertexBufferCount_,
                                      element.GetComponentCount(),
                                      GL_FLOAT,
                                      element.normalized ? GL_TRUE : GL_FALSE,
                                      layout.GetStride(),
                                      (const void*)(uintptr_t)element.offset);
                vertexBufferCount_++;
                break;
            }
            case Renderer::ShaderDataType::Int:
            case Renderer::ShaderDataType::Int2:
            case Renderer::ShaderDataType::Int3:
            case Renderer::ShaderDataType::Int4: {
                glEnableVertexAttribArray(vertexBufferCount_);
                glVertexAttribIPointer(vertexBufferCount_,
                                       element.GetComponentCount(),
                                       GL_INT,
                                       layout.GetStride(),
                                       (const void*)(uintptr_t)element.offset);
                vertexBufferCount_++;
                break;
            }
            case Renderer::ShaderDataType::Mat3:
            case Renderer::ShaderDataType::Mat4: {
                uint8_t count = element.GetComponentCount();
                for (uint8_t i = 0; i < count; i++)
                {
                    glEnableVertexAttribArray(vertexBufferCount_);
                    glVertexAttribPointer(vertexBufferCount_,
                                          count,
                                          GL_FLOAT,
                                          element.normalized
                                          ? GL_TRUE
                                          : GL_FALSE,
                                          layout.GetStride(),
                                          (const void*)(uintptr_t)(element.
                                              offset + sizeof(
                                                  float) * count * i));
                    vertexBufferCount_++;
                }
                break;
            }
            case Renderer::ShaderDataType::Bool: {
                glEnableVertexAttribArray(vertexBufferCount_);
                glVertexAttribIPointer(vertexBufferCount_,
                                       element.GetComponentCount(),
                                       GL_INT,
                                       layout.GetStride(),
                                       (const void*)(uintptr_t)element.offset);
                vertexBufferCount_++;
                break;
            }
            default:
                // TODO ASSERT
                GEF_ENGINE_ERROR("Unsupported data type");
            }
        }
        vertexBuffers_.push_back(vertexBuffer);
    }

    void OpenGlVertexArray::SetIndexBuffer(
        const std::shared_ptr<Renderer::IndexBuffer>& indexBuffer)
    {
        glBindVertexArray(id_);
        indexBuffer->Bind();
        indexBuffer_ = indexBuffer;
    }

    const std::vector<std::shared_ptr<Renderer::VertexBuffer> >&
    OpenGlVertexArray::GetVertexBuffers() const
    {
        return vertexBuffers_;
    }

    const std::shared_ptr<Renderer::IndexBuffer>& OpenGlVertexArray::
    GetIndexBuffer() const
    {
        return indexBuffer_;
    }
}
