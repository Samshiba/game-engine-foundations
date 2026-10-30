//
// Created by genin on 27/10/2026.
// Path: Engine/src/Engine/Renderer/CommandList.cpp
//

#include <algorithm>

#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Core/Assert.hpp>

namespace GEF::Renderer
{
    void CommandList::Reset()
    {
        frameCommands_.clear();
        drawPackets_.clear();
        sortKeys_.clear();
        pipeline_ = PipelineHandle();
        vertexArray_ = VertexArrayHandle();
        uniformBuffers_.fill(BufferHandle{});
    }

    void CommandList::Clear()
    {
        frameCommands_.emplace_back(ClearCommand{});
    }

    void CommandList::SetClearColor(float r, float g, float b, float a)
    {
        frameCommands_.emplace_back(SetClearColorCommand{ r, g, b, a });
    }

    void CommandList::SetViewport(uint32_t x, uint32_t y, uint32_t width,
                                  uint32_t height)
    {
        frameCommands_.emplace_back(SetViewportCommand{ x, y, width, height });
    }

    void CommandList::BindVertexArray(VertexArrayHandle handle)
    {
        vertexArray_ = handle;
    }

    void CommandList::BindPipeline(PipelineHandle handle)
    {
        pipeline_ = handle;
    }

    void CommandList::BindUniformBuffer(uint32_t bindingPoint,
                                        BufferHandle handle)
    {
        GEF_CORE_ASSERT(bindingPoint < MaxUniformBindings,
                        "bindingPoint out of range");
        uniformBuffers_[bindingPoint] = handle;
    }

    void CommandList::DrawIndexed(uint32_t indexCount)
    {
        DrawIndexedInstanced(indexCount, 1, 0);
    }

    void CommandList::DrawIndexedInstanced(uint32_t indexCount,
                                           uint32_t instanceCount,
                                           uint32_t baseInstance)
    {
        GEF_CORE_ASSERT(pipeline_.IsValid() && vertexArray_.IsValid(),
                        "Draw without a bound pipeline or vertex array");
        GEF_CORE_ASSERT(pipeline_.index <= SortKey::FIELD_MASK_12,
                        "Pipeline index out of bounds");
        GEF_CORE_ASSERT(vertexArray_.index <= SortKey::FIELD_MASK_12,
                        "VertexArray index out of bounds");
        GEF_CORE_ASSERT(drawPackets_.size() <= SortKey::SEQUENCE_MASK,
                        "Too many draw packets");

        sortKeys_.emplace_back(SortKey::Make(0, pipeline_.index,
                                             vertexArray_.index, 0,
                                             static_cast<uint32_t>(drawPackets_.
                                                 size())));

        drawPackets_.emplace_back(pipeline_, vertexArray_, uniformBuffers_,
                                  indexCount, instanceCount, baseInstance);
    }

    void CommandList::SortPackets()
    {
        std::ranges::sort(sortKeys_);
    }

    const std::vector<CommandList::FrameCommand>& CommandList::
    GetFrameCommands() const
    {
        return frameCommands_;
    }

    const std::vector<CommandList::DrawPacket>& CommandList::
    GetDrawPackets() const
    {
        return drawPackets_;
    }

    const std::vector<uint64_t>& CommandList::GetSortKeys() const
    {
        return sortKeys_;
    }
}