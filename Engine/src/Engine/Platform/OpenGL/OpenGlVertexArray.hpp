//
// Created by genin on 09/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGlVertexArray.hpp
//

#pragma once

#include <Engine/Renderer/VertexArray.hpp>

namespace GEF::Platform
{
    class OpenGlVertexArray : public Renderer::VertexArray
    {
    public:
        ~OpenGlVertexArray() override;
        OpenGlVertexArray();

        void Bind() const override;
        void Unbind() const override;

        void AddVertexBuffer(
            const std::shared_ptr<Renderer::VertexBuffer>& vertexBuffer)
        override;
        void SetIndexBuffer(
            const std::shared_ptr<Renderer::IndexBuffer>& indexBuffer) override;

        [[nodiscard]] const std::vector<std::shared_ptr<
            Renderer::VertexBuffer> >
        & GetVertexBuffers() const override;
        [[nodiscard]] const std::shared_ptr<Renderer::IndexBuffer>&
        GetIndexBuffer() const override;

    private:
        unsigned int id_;
        unsigned int vertexBufferCount_ = 0;
        std::vector<std::shared_ptr<Renderer::VertexBuffer> > vertexBuffers_;
        std::shared_ptr<Renderer::IndexBuffer> indexBuffer_;
    };
}