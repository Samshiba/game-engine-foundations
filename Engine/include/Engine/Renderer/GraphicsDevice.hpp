//
// Created by genin on 15/02/2026.
// Path: Engine/include/Engine/Renderer/GraphicsDevice.hpp
//

#pragma once

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Buffer.hpp"
#include "CommandList.hpp"
#include "Handle.hpp"
#include "RendererAPI.hpp"

namespace GEF::Renderer
{
    // Buffer
    enum class BufferType : uint8_t
    {
        Index,
        Vertex,
        Uniform,
        ShaderStorage,
    };

    enum class BufferUsage : uint8_t
    {
        Static,
        Dynamic,
        Stream
    };

    struct BufferDesc
    {
        BufferType type;
        BufferUsage usage = BufferUsage::Static;
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

    // Pipeline
    enum class CullMode : uint8_t
    {
        None,
        Back,
        Front,
    };

    struct DepthState
    {
        bool test = true;
        bool write = true;
    };

    struct PipelineDesc
    {
        ShaderHandle shader;
        DepthState depth;
        CullMode cull = CullMode::Back;
    };

    // Texture
    enum class TextureFormat : uint8_t
    {
        None,
        RGBA8,
        SRGBA8,
        Depth32F
    };

    enum class TextureWrap : uint8_t
    {
        ClampToEdge,
        ClampToBorder,
        Repeat,
        MirroredRepeat,
        MirrorClampToEdge,
    };

    struct TextureDesc
    {
        uint32_t width = 0;
        uint32_t height = 0;
        bool generateMipmaps = true;
        TextureFormat format = TextureFormat::RGBA8;
        TextureWrap wrap = TextureWrap::ClampToEdge;
        std::array<float, 4> borderColor = { 0.0f, 0.0f, 0.0f, 0.0f };
        const void* data = nullptr;
    };

    // Passes
    static constexpr uint32_t MaxColorAttachments = 4;

    enum class SizeMode : uint8_t
    {
        Fixed,
        WindowRelative
    };

    struct RenderTargetDesc
    {
        SizeMode sizeMode = SizeMode::Fixed;
        uint32_t width = 1;
        uint32_t height = 1;
        float scaleX = 1.0f;
        float scaleY = 1.0f;

        std::array<TextureFormat, MaxColorAttachments> attachments = {
            TextureFormat::None
        };
        TextureFormat depthStencil = TextureFormat::None;
    };

    // Infos
    struct AdapterInfo
    {
        std::string vendor;
        std::string renderer; // the GPU model
        std::string version; // API + driver version
    };

    struct SubmitStats
    {
        uint32_t drawCalls = 0;
        uint32_t pipelineChanges = 0;
        uint32_t vertexArrayChanges = 0;
        uint32_t uniformBufferChanges = 0;
        uint32_t shaderStorageBufferChanges = 0;
        uint32_t textureChanges = 0;
        double gpuFrameTimeMs = 0;
        // CPU time spent issuing the same commands (the sort excluded).
        // Compared with gpuFrameTimeMs, the larger one limits Submit: CPU,
        // the driver cannot take the draw calls fast enough; GPU, the CPU
        // is done long before the GPU.
        double cpuSubmitTimeMs = 0;
    };

    class GraphicsDevice
    {
    protected:
        static constexpr uint8_t GpuTimerCount = 3;

    public:
        virtual ~GraphicsDevice() = default;

        // Buffer
        [[nodiscard]] virtual BufferHandle
        CreateBuffer(const BufferDesc& desc) = 0;
        virtual void DestroyBuffer(BufferHandle handle) = 0;
        virtual bool UpdateBuffer(BufferHandle handle, uint32_t offset,
                                  const void* data, uint32_t size) = 0;

        // Vertex Array
        [[nodiscard]] virtual VertexArrayHandle
        CreateVertexArray(const VertexArrayDesc& desc) = 0;
        virtual void DestroyVertexArray(VertexArrayHandle handle) = 0;

        // Shader
        [[nodiscard]] virtual ShaderHandle
        CreateShader(const ShaderDesc& desc) = 0;
        virtual void DestroyShader(ShaderHandle handle) = 0;

        // Pipeline
        [[nodiscard]] virtual PipelineHandle
        CreatePipeline(const PipelineDesc& desc) = 0;
        virtual void DestroyPipeline(PipelineHandle handle) = 0;

        // Texture
        [[nodiscard]] virtual TextureHandle
        CreateTexture(const TextureDesc& desc) = 0;
        virtual void DestroyTexture(TextureHandle handle) = 0;

        // Render Target
        [[nodiscard]] virtual RenderTargetHandle
        CreateRenderTarget(const RenderTargetDesc& desc) = 0;
        virtual void DestroyRenderTarget(RenderTargetHandle handle) = 0;
        [[nodiscard]] virtual TextureHandle
        GetRenderTargetDepth(RenderTargetHandle handle) = 0;

        // Infos
        [[nodiscard]] virtual const AdapterInfo& GetAdapterInfo() const = 0;
        [[nodiscard]] virtual const SubmitStats& GetLastSubmitStats() const = 0;
        [[nodiscard]] virtual std::vector<uint8_t>
        ReadFramebuffer(uint32_t width, uint32_t height) const = 0;

        static std::unique_ptr<GraphicsDevice> Create(RendererBackend backend);

        virtual void Submit(CommandList& cmd) = 0;
        virtual void SetBackBufferSize(uint32_t width, uint32_t height) = 0;
    };
} // namespace GEF::Renderer