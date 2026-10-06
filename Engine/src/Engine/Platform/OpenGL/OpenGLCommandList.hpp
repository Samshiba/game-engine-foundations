//
// Created by genin on 16/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLCommandList.hpp
//

#pragma once

#include <Engine/Renderer/CommandList.hpp>
#include <glad/glad.h>

#include "OpenGLGraphicsDevice.hpp"

namespace GEF::Platform
{
    class OpenGLCommandList : public Renderer::CommandList
    {
    public:
        explicit OpenGLCommandList(const OpenGLGraphicsDevice& device);

        void SetClearColor(float r, float g, float b, float a) override;
        void Clear() override;

        void BindVertexArray(Renderer::VertexArrayHandle handle) override;
        void BindPipeline(Renderer::PipelineHandle handle) override;
        void BindUniformBuffer(uint32_t bindingPoint,
                               Renderer::BufferHandle handle) override;

        void DrawIndexed(uint32_t indexCount) override;

    private:
        const OpenGLGraphicsDevice& device_;
    };

    inline OpenGLCommandList::OpenGLCommandList(
        const OpenGLGraphicsDevice& device)
        : device_(device)
    {
    }

    inline void OpenGLCommandList::SetClearColor(float r, float g, float b,
                                                 float a)
    {
        glClearColor(r, g, b, a);
    }

    inline void OpenGLCommandList::Clear()
    {
        glDepthMask(GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    inline void
    OpenGLCommandList::BindVertexArray(Renderer::VertexArrayHandle handle)
    {
        const GLVertexArray* vertexArray = device_.GetVertexArray(handle);
        GEF_CORE_ASSERT(vertexArray, "Invalid vertex array handle");
        glBindVertexArray(vertexArray->id);
    }

    inline void OpenGLCommandList::BindPipeline(Renderer::PipelineHandle handle)
    {
        const GLPipeline* pipeline = device_.GetPipeline(handle);
        GEF_CORE_ASSERT(pipeline, "Invalid pipeline handle");

        auto shader = device_.GetShader(pipeline->shader);
        GEF_CORE_ASSERT(shader, "Invalid shader handle");

        if (!shader)
            return;

        glUseProgram(shader->program);
        if (pipeline->depth.test)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
        glDepthMask(pipeline->depth.write);

        switch (pipeline->cull)
        {
        case Renderer::CullMode::None:
            glDisable(GL_CULL_FACE);
            break;
        case Renderer::CullMode::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case Renderer::CullMode::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        default:
            glDisable(GL_CULL_FACE);
        }
    }

    inline void OpenGLCommandList::BindUniformBuffer(uint32_t bindingPoint,
        Renderer::BufferHandle handle)
    {
        const GLBuffer* buffer = device_.GetBuffer(handle);
        GEF_CORE_ASSERT(buffer, "Invalid buffer handle");
        GEF_CORE_ASSERT(buffer->type == Renderer::BufferType::Uniform,
                        "Buffer is not a uniform buffer");
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, buffer->id);
    }

    inline void OpenGLCommandList::DrawIndexed(uint32_t indexCount)
    {
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                       GL_UNSIGNED_INT, nullptr);
    }
} // namespace GEF::Platform