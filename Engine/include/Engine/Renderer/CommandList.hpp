//
// Created by genin on 16/02/2026.
// Path: Engine/include/Engine/Renderer/CommandList.hpp
//

#pragma once

#include <array>
#include <cstdint>
#include <variant>
#include <vector>

#include <Engine/Renderer/Handle.hpp>
#include <Engine/Renderer/LightingData.hpp>

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
    } // namespace SortKey

    class CommandList
    {
    public:
        struct DrawPacket
        {
            PipelineHandle pipeline;
            VertexArrayHandle vertexArray;
            std::array<BufferHandle, MaxUniformBindings> uniformBuffers;
            uint32_t indexCount;
            uint32_t instanceCount;
            uint32_t baseInstance;
        };

        struct ClearCommand
        {};

        struct SetClearColorCommand
        {
            float r;
            float g;
            float b;
            float a;
        };

        struct SetViewportCommand
        {
            uint32_t x;
            uint32_t y;
            uint32_t width;
            uint32_t height;
        };

        using FrameCommand = std::variant<SetClearColorCommand, ClearCommand,
                                          SetViewportCommand>;

    public:
        ~CommandList() = default;

        void Reset();
        void SortPackets();

        // Frame Commands
        void Clear();
        void SetClearColor(float r, float g, float b, float a);
        void SetViewport(uint32_t x, uint32_t y, uint32_t width,
                         uint32_t height);

        // Packets
        void BindVertexArray(VertexArrayHandle handle);
        void BindPipeline(PipelineHandle handle);
        void BindUniformBuffer(uint32_t bindingPoint, BufferHandle handle);

        void DrawIndexed(uint32_t indexCount);
        void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                  uint32_t baseInstance);

        // Getters for backend
        [[nodiscard]] const std::vector<FrameCommand>& GetFrameCommands() const;
        [[nodiscard]] const std::vector<DrawPacket>& GetDrawPackets() const;
        [[nodiscard]] const std::vector<uint64_t>& GetSortKeys() const;

    private:
        std::vector<FrameCommand> frameCommands_;

        std::vector<DrawPacket> drawPackets_;
        std::vector<uint64_t> sortKeys_;

        // Cache
        PipelineHandle pipeline_;
        VertexArrayHandle vertexArray_;
        std::array<BufferHandle, MaxUniformBindings> uniformBuffers_;
    };
} // namespace GEF::Renderer