//
// Created by genin on 27/10/2026.
// Path: Engine/src/Engine/Renderer/CommandList.cpp
//

#include <Engine/Renderer/CommandList.hpp>

#include <algorithm>

#include <Engine/Core/Assert.hpp>

namespace GEF::Renderer
{
    CommandList::CommandList()
    {
        Reset();
    }

    void CommandList::Reset()
    {
        passes_.clear();
        inPass_ = false;
        isImplicitPass_ = false;
        drawPackets_.clear();
        sortKeys_.clear();
        pipeline_ = PipelineHandle();
        vertexArray_ = VertexArrayHandle();
        uniformBuffers_.fill(BufferHandle{});
        shaderStorageBuffers_.fill(BufferHandle{});
        textures_.fill(TextureHandle{});

        passes_.push_back(
            { .desc = { .clearValue = { 0.1f, 0.1f, 0.1f, 1 } } });
        inPass_ = true;
        isImplicitPass_ = true;
    }

    void CommandList::SortPackets()
    {
        std::ranges::sort(sortKeys_);
    }

    void CommandList::Blit(RenderTargetHandle handle)
    {
        GEF_CORE_ASSERT(inPass_, "Blit outside a pass");
        passes_.back().blitSource = handle;
    }

    void CommandList::BeginPass(const PassDesc& desc)
    {
        GEF_CORE_ASSERT(!inPass_ || isImplicitPass_, "BeginPass inside a pass");
        GEF_CORE_ASSERT(passes_.size() <= SortKey::PASS_MASK,
                        "Too many passes");
        passes_.push_back({ .desc = desc });
        inPass_ = true;
        isImplicitPass_ = false;
    }

    void CommandList::EndPass()
    {
        GEF_CORE_ASSERT(inPass_ && !isImplicitPass_, "EndPass outside a pass");
        inPass_ = false;
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

    void CommandList::BindShaderStorageBuffer(uint32_t bindingPoint,
                                              BufferHandle handle)
    {
        GEF_CORE_ASSERT(bindingPoint < MaxShaderStorageBindings,
                        "bindingPoint out of range");
        shaderStorageBuffers_[bindingPoint] = handle;
    }

    void CommandList::BindTexture(uint32_t unit, TextureHandle handle)
    {
        GEF_CORE_ASSERT(unit < MaxTextureBindings, "unit out of range");
        textures_[unit] = handle;
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
        GEF_CORE_ASSERT(inPass_, "Attempting to draw outside of a pass");

        sortKeys_.emplace_back(SortKey::Make(
            static_cast<uint32_t>(passes_.size() - 1), pipeline_.index,
            vertexArray_.index, 0, static_cast<uint32_t>(drawPackets_.size())));

        drawPackets_.emplace_back(pipeline_, vertexArray_, uniformBuffers_,
                                  shaderStorageBuffers_, textures_, indexCount,
                                  instanceCount, baseInstance);
    }

    const std::vector<CommandList::Pass>& CommandList::GetPasses() const
    {
        return passes_;
    }

    const std::vector<CommandList::DrawPacket>&
    CommandList::GetDrawPackets() const
    {
        return drawPackets_;
    }

    const std::vector<uint64_t>& CommandList::GetSortKeys() const
    {
        return sortKeys_;
    }
} // namespace GEF::Renderer