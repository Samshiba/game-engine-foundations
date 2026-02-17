//
// Created by genin on 15/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp
//

#pragma once

#include <Engine/Renderer/GraphicsDevice.hpp>

#include "OpenGlIndexBuffer.hpp"
#include "OpenGlVertexBuffer.hpp"
#include "OpenGlVertexArray.hpp"

namespace GEF::Platform
{
    class OpenGLGraphicsDevice : public Renderer::GraphicsDevice
    {
    public:
        OpenGLGraphicsDevice();

        [[nodiscard]] std::shared_ptr<Renderer::IndexBuffer>
        CreateIndexBuffer(uint32_t* indices, uint32_t count) override;
        [[nodiscard]] std::shared_ptr<Renderer::VertexBuffer>
        CreateVertexBuffer(float* vertices, uint32_t size) override;
        [[nodiscard]] std::shared_ptr<Renderer::VertexArray> CreateVertexArray()
        override;

        std::shared_ptr<Renderer::CommandList> BeginCommandList() override;
        void SubmitCommandList(
            const std::shared_ptr<Renderer::CommandList>& commandList) override;
    };
}