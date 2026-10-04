//
// Created by genin on 15/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp
//

#pragma once

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/ResourcePool.hpp>

#include <glad/glad.h>

namespace GEF::Platform
{
    struct GLBuffer
    {
        GLuint id;
        uint32_t size;
        Renderer::BufferType type;
    };

    struct GLVertexArray
    {
        GLuint id;
    };

    class OpenGLGraphicsDevice : public Renderer::GraphicsDevice
    {
    public:
        OpenGLGraphicsDevice() = default;
        ~OpenGLGraphicsDevice() override;

        [[nodiscard]] Renderer::BufferHandle CreateBuffer(
            const Renderer::BufferDesc& desc) override;
        void DestroyBuffer(Renderer::BufferHandle handle) override;

        // Backend-side lookup (command list execution, tests), nullptr if stale
        [[nodiscard]] const GLBuffer* GetBuffer(
            Renderer::BufferHandle handle) const;
        [[nodiscard]] const GLVertexArray* GetVertexArray(
            Renderer::VertexArrayHandle handle) const;

        [[nodiscard]] Renderer::VertexArrayHandle CreateVertexArray(
            const Renderer::VertexArrayDesc& desc) override;
        void DestroyVertexArray(Renderer::VertexArrayHandle handle) override;

        std::shared_ptr<Renderer::CommandList> BeginCommandList() override;
        void SubmitCommandList(
            const std::shared_ptr<Renderer::CommandList>& commandList) override;

    private:
        Renderer::ResourcePool<GLBuffer, Renderer::BufferTag> buffers_;
        Renderer::ResourcePool<GLVertexArray, Renderer::VertexArrayTag>
        vertexArrays_;
    };
}