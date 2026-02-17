//
// Created by genin on 16/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLCommandList.hpp
//

#pragma once

#include <Engine/Renderer/CommandList.hpp>

#include <glad/glad.h>

namespace GEF::Platform
{
    class OpenGLCommandList : public Renderer::CommandList
    {
    public:
        void SetClearColor(float r, float g, float b, float a) override;
        void Clear() override;

        void BindVertexArray(
            const std::shared_ptr<Renderer::VertexArray>& vertexArray) override;
        void DrawIndexed(uint32_t indexCount) override;
    };

    inline void OpenGLCommandList::SetClearColor(float r, float g, float b,
                                                 float a)
    {
        glClearColor(r, g, b, a);
    }

    inline void OpenGLCommandList::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    inline void OpenGLCommandList::BindVertexArray(
        const std::shared_ptr<Renderer::VertexArray>& vertexArray)
    {
        vertexArray->Bind();
    }

    inline void OpenGLCommandList::DrawIndexed(uint32_t indexCount)
    {
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    }
}