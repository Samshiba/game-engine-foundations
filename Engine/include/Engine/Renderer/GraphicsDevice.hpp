//
// Created by genin on 15/02/2026.
// Path: Engine/include/Engine/Renderer/GraphicsDevice.hpp
//

#pragma once

#include "RendererAPI.hpp"
#include "CommandList.hpp"
#include "Buffer.hpp"
#include "Handle.hpp"

#include <memory>
#include <vector>
#include <string_view>

namespace GEF::Renderer
{
    // Buffer
    enum class BufferType
    {
        Index,
        Vertex,
        Uniform,
    };

    enum class BufferUsage
    {
        Static,
        Dynamic,
        Stream
    };

    struct BufferDesc
    {
        BufferType type;
        BufferUsage usage;
        uint32_t size = 0;
        const void* data = nullptr;
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

    // Shader
    struct ShaderDesc
    {
        std::string_view vertexSource;
        std::string_view fragmentSource;
    };

    class GraphicsDevice
    {
    public:
        virtual ~GraphicsDevice() = default;

        // Buffer
        [[nodiscard]] virtual BufferHandle CreateBuffer(const BufferDesc& desc)
        = 0;
        virtual void DestroyBuffer(BufferHandle handle) = 0;
        virtual bool UpdateBuffer(BufferHandle handle, uint32_t offset,
                                  const void* data,
                                  uint32_t size) = 0;

        // vertex Array
        [[nodiscard]] virtual VertexArrayHandle CreateVertexArray(
            const VertexArrayDesc& desc) = 0;
        virtual void DestroyVertexArray(VertexArrayHandle handle) = 0;

        // Shader
        [[nodiscard]] virtual ShaderHandle CreateShader(const ShaderDesc& desc)
        = 0;
        virtual void DestroyShader(ShaderHandle handle) = 0;

        static std::unique_ptr<GraphicsDevice> Create(RendererBackend backend);

        virtual std::shared_ptr<CommandList> BeginCommandList() = 0;
        virtual void SubmitCommandList(
            const std::shared_ptr<CommandList>& commandList) = 0;
    };
}