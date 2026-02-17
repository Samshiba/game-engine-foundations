//
// Created by genin on 09/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGlVertexBuffer.hpp
//

#pragma once

#include <Engine/Renderer/VertexBuffer.hpp>

namespace GEF::Platform
{
    class OpenGLVertexBuffer : public Renderer::VertexBuffer
    {
    public:
        ~OpenGLVertexBuffer() override;
        OpenGLVertexBuffer(float* vertices, uint32_t size);

        void Bind() const override;
        void Unbind() const override;

        void SetLayout(const Renderer::BufferLayout& layout) override;
        [[nodiscard]] const Renderer::BufferLayout& GetLayout() const override;

    private:
        unsigned int id_;
        Renderer::BufferLayout layout_;
    };
}