//
// Created by genin on 15/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp
//

#pragma once

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/ResourcePool.hpp>

#include "OpenGlVertexArray.hpp"

#include <glad/glad.h>

namespace GEF::Platform
{
    struct GLBuffer
    {
        GLuint id;
        uint32_t size;
        Renderer::BufferType type;
    };

    class OpenGLGraphicsDevice : public Renderer::GraphicsDevice
    {
    public:
        OpenGLGraphicsDevice() = default;
        ~OpenGLGraphicsDevice() override;

        [[nodiscard]] std::shared_ptr<Renderer::IndexBuffer>
        CreateIndexBuffer(uint32_t* indices, uint32_t count) override;
        [[nodiscard]] std::shared_ptr<Renderer::VertexBuffer>
        CreateVertexBuffer(float* vertices, uint32_t size) override;

        [[nodiscard]] Renderer::BufferHandle CreateBuffer(
            const Renderer::BufferDesc& desc) override;
        void DestroyBuffer(Renderer::BufferHandle handle) override;

        // Backend-side lookup (command list execution, tests), nullptr if stale
        [[nodiscard]] const GLBuffer* GetBuffer(
            Renderer::BufferHandle handle) const;

        [[nodiscard]] std::shared_ptr<Renderer::VertexArray> CreateVertexArray()
        override;

        std::shared_ptr<Renderer::CommandList> BeginCommandList() override;
        void SubmitCommandList(
            const std::shared_ptr<Renderer::CommandList>& commandList) override;

    private:
        Renderer::ResourcePool<GLBuffer, Renderer::BufferTag> buffers_;
    };
}