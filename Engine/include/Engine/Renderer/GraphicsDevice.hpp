//
// Created by genin on 15/02/2026.
// Path: Engine/include/Engine/Renderer/GraphicsDevice.hpp
//

#pragma once

#include "IndexBuffer.hpp"
#include "VertexBuffer.hpp"
#include "VertexArray.hpp"
#include "RendererAPI.hpp"
#include "CommandList.hpp"
#include "Buffer.hpp"
#include "Handle.hpp"

#include <memory>
#include <vector>

namespace GEF::Renderer
{
    // Buffer
    enum class BufferType
    {
        Index,
        Vertex
    };

    struct BufferDesc
    {
        BufferType type;
        uint32_t size;

        const void* data;
    };

    // Vertex Array
    struct VertexBufferBinding
    {
        BufferHandle buffer;
        BufferLayout layout;
        uint32_t instanceDivisor = 0;
    };

    struct VertexArrayDesc
    {
        std::vector<VertexBufferBinding> vertexBuffers;
        BufferHandle indexBuffer;
    };

    class GraphicsDevice
    {
    public:
        virtual ~GraphicsDevice() = default;

        // Buffer
        [[nodiscard]] virtual std::shared_ptr<IndexBuffer> CreateIndexBuffer(
            uint32_t* indices, uint32_t count) = 0;
        [[nodiscard]] virtual std::shared_ptr<VertexBuffer>
        CreateVertexBuffer(float* vertices, uint32_t size) = 0;

        [[nodiscard]] virtual BufferHandle CreateBuffer(const BufferDesc& desc)
        = 0;
        virtual void DestroyBuffer(BufferHandle handle) = 0;


        // Vertex Array
        [[nodiscard]] virtual std::shared_ptr<VertexArray> CreateVertexArray() =
        0;
        [[nodiscard]] virtual VertexArrayHandle CreateVertexArray(
            const VertexArrayDesc& desc) = 0;
        virtual void DestroyVertexArray(VertexArrayHandle handle) = 0;

        static std::unique_ptr<GraphicsDevice> Create(RendererBackend backend);

        virtual std::shared_ptr<CommandList> BeginCommandList() = 0;
        virtual void SubmitCommandList(
            const std::shared_ptr<CommandList>& commandList) = 0;
    };
}