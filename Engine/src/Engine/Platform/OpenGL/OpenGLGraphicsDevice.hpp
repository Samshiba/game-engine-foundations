//
// Created by genin on 15/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp
//

#pragma once

#include <array>

#include <glad/glad.h>

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/ResourcePool.hpp>

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

    struct GLShader
    {
        GLuint program;
    };

    struct GLPipeline
    {
        Renderer::ShaderHandle shader;
        Renderer::DepthState depth;
        Renderer::CullMode cull;
    };

    struct GLTexture
    {
        GLuint id;
        uint32_t width;
        uint32_t height;
        GLenum format;
        GLint level;
    };

    class OpenGLGraphicsDevice : public Renderer::GraphicsDevice
    {
    private:
        struct BoundState
        {
            Renderer::PipelineHandle pipeline;
            Renderer::VertexArrayHandle vertexArray;
            std::array<Renderer::BufferHandle,
                       Renderer::CommandList::MaxUniformBindings>
                uniformBuffers;
            std::array<Renderer::BufferHandle,
                       Renderer::CommandList::MaxShaderStorageBindings>
                shaderStorageBuffers;
            std::array<Renderer::TextureHandle,
                       Renderer::CommandList::MaxTextureBindings>
                textures;
        };

        struct GpuTimer
        {
            GLuint start = 0;
            GLuint end = 0;
            bool pending = false; // issued, result not read yet
        };

    public:
        OpenGLGraphicsDevice();
        ~OpenGLGraphicsDevice() override;

        // Buffer
        [[nodiscard]] Renderer::BufferHandle
        CreateBuffer(const Renderer::BufferDesc& desc) override;
        void DestroyBuffer(Renderer::BufferHandle handle) override;
        bool UpdateBuffer(Renderer::BufferHandle handle, uint32_t offset,
                          const void* data, uint32_t size) override;

        // Vertex Array
        [[nodiscard]] Renderer::VertexArrayHandle
        CreateVertexArray(const Renderer::VertexArrayDesc& desc) override;
        void DestroyVertexArray(Renderer::VertexArrayHandle handle) override;

        // Shader
        [[nodiscard]] Renderer::ShaderHandle
        CreateShader(const Renderer::ShaderDesc& desc) override;
        void DestroyShader(Renderer::ShaderHandle handle) override;

        // Pipeline
        [[nodiscard]] Renderer::PipelineHandle
        CreatePipeline(const Renderer::PipelineDesc& desc) override;
        void DestroyPipeline(Renderer::PipelineHandle handle) override;

        // Texture
        [[nodiscard]] Renderer::TextureHandle
        CreateTexture(const Renderer::TextureDesc& desc) override;
        void DestroyTexture(Renderer::TextureHandle handle) override;

        // Backend-side lookup (command list execution, tests), nullptr if stale
        [[nodiscard]] const GLBuffer*
        GetBuffer(Renderer::BufferHandle handle) const;
        [[nodiscard]] const GLVertexArray*
        GetVertexArray(Renderer::VertexArrayHandle handle) const;
        [[nodiscard]] const GLShader*
        GetShader(Renderer::ShaderHandle handle) const;
        [[nodiscard]] const GLPipeline*
        GetPipeline(Renderer::PipelineHandle handle) const;

        // Infos
        [[nodiscard]] const Renderer::AdapterInfo&
        GetAdapterInfo() const override;
        [[nodiscard]] const Renderer::SubmitStats&
        GetLastSubmitStats() const override;
        [[nodiscard]] std::vector<uint8_t>
        ReadFramebuffer(uint32_t width, uint32_t height) const override;

        void Submit(Renderer::CommandList& cmd) override;

    private:
        void SetClearColor(float r, float g, float b, float a);
        void Clear();
        void SetViewport(uint32_t x, uint32_t y, uint32_t width,
                         uint32_t height);
        void BindVertexArray(Renderer::VertexArrayHandle handle);
        void BindPipeline(Renderer::PipelineHandle handle);
        void BindUniformBuffer(uint32_t bindingPoint,
                               Renderer::BufferHandle handle);
        void BindShaderStorageBuffer(uint32_t bindingPoint,
                                     Renderer::BufferHandle handle);
        void BindTexture(uint32_t unit, Renderer::TextureHandle handle);

        void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                  uint32_t baseInstance);

    private:
        // Infos
        Renderer::AdapterInfo adapterInfo_;
        Renderer::SubmitStats lastSubmitStats_;
        std::array<GpuTimer, GpuTimerCount> gpuTimers_{};
        std::size_t gpuTimerIndex_ = 0;
        double gpuFrameTimeMs_ = 0;

        // Pools
        Renderer::ResourcePool<GLBuffer, Renderer::BufferTag> buffers_;
        Renderer::ResourcePool<GLVertexArray, Renderer::VertexArrayTag>
            vertexArrays_;
        Renderer::ResourcePool<GLShader, Renderer::ShaderTag> shaders_;
        Renderer::ResourcePool<GLPipeline, Renderer::PipelineTag> pipelines_;
        Renderer::ResourcePool<GLTexture, Renderer::TextureTag> textures_;
    };
} // namespace GEF::Platform