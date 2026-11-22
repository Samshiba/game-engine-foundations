//
// Created by genin on 16/02/2026.
// Path: Engine/include/Engine/Renderer/CommandList.hpp
//

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include <Engine/Renderer/Handle.hpp>

namespace GEF::Renderer
{
    namespace SortKey
    {
        inline constexpr uint32_t PASS_SHIFT = 60;
        inline constexpr uint32_t PIPELINE_SHIFT = 48;
        inline constexpr uint32_t VERTEXARRAY_SHIFT = 36;
        inline constexpr uint32_t DEPTH_SHIFT = 20;

        inline constexpr uint64_t FIELD_MASK_12 = 0xFFF;
        inline constexpr uint64_t SEQUENCE_MASK = 0xFFFFF;
        inline constexpr uint64_t PASS_MASK = 0xF;

        constexpr uint64_t Make(uint32_t pass, uint32_t pipeline,
                                uint32_t vertexArray, uint32_t depth,
                                uint32_t sequence)
        {
            return static_cast<uint64_t>(pass) << PASS_SHIFT
                | static_cast<uint64_t>(pipeline) << PIPELINE_SHIFT
                | static_cast<uint64_t>(vertexArray) << VERTEXARRAY_SHIFT
                | static_cast<uint64_t>(depth) << DEPTH_SHIFT
                | static_cast<uint64_t>(sequence);
        }

        constexpr uint32_t Sequence(uint64_t key)
        {
            return uint32_t(key & SEQUENCE_MASK);
        }

        constexpr uint32_t Pass(uint64_t key)
        {
            return uint32_t((key >> PASS_SHIFT) & PASS_MASK);
        }
    } // namespace SortKey

    class CommandList
    {
    public:
        static constexpr uint32_t MaxUniformBindings = 3;
        static constexpr uint32_t MaxShaderStorageBindings = 3;
        static constexpr uint32_t MaxTextureBindings = 3;

        struct DrawPacket
        {
            PipelineHandle pipeline;
            VertexArrayHandle vertexArray;
            std::array<BufferHandle, MaxUniformBindings> uniformBuffers;
            std::array<BufferHandle, MaxShaderStorageBindings>
                shaderStorageBuffers;
            std::array<TextureHandle, MaxTextureBindings> textures;
            uint32_t indexCount;
            uint32_t instanceCount;
            uint32_t baseInstance;
        };

        struct PassDesc
        {
            RenderTargetHandle target{}; // invalide = back buffer
            bool clearColor = true;
            bool clearDepth = true;
            glm::vec4 clearValue{ 0, 0, 0, 1 };
        };

        struct Pass
        {
            PassDesc desc;
            RenderTargetHandle blitSource{};
        };

    public:
        CommandList();
        ~CommandList() = default;

        void Reset();
        void SortPackets();
        void Blit(RenderTargetHandle handle);

        // Passes
        void BeginPass(const PassDesc& desc);
        void EndPass();

        // Packets
        void BindVertexArray(VertexArrayHandle handle);
        void BindPipeline(PipelineHandle handle);
        void BindUniformBuffer(uint32_t bindingPoint, BufferHandle handle);
        void BindShaderStorageBuffer(uint32_t bindingPoint,
                                     BufferHandle handle);
        void BindTexture(uint32_t unit, TextureHandle handle);

        void DrawIndexed(uint32_t indexCount);
        void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                  uint32_t baseInstance);

        // Getters for backend
        [[nodiscard]] const std::vector<Pass>& GetPasses() const;
        [[nodiscard]] const std::vector<DrawPacket>& GetDrawPackets() const;
        [[nodiscard]] const std::vector<uint64_t>& GetSortKeys() const;

    private:
        bool inPass_ = false;
        bool isImplicitPass_ = false;
        std::vector<Pass> passes_;

        std::vector<DrawPacket> drawPackets_;
        std::vector<uint64_t> sortKeys_;

        // Cache
        PipelineHandle pipeline_;
        VertexArrayHandle vertexArray_;
        std::array<BufferHandle, MaxUniformBindings> uniformBuffers_;
        std::array<BufferHandle, MaxShaderStorageBindings>
            shaderStorageBuffers_;
        std::array<TextureHandle, MaxTextureBindings> textures_;
    };
} // namespace GEF::Renderer