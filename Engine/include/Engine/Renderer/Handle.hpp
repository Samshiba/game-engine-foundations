//
// Created by genin on 04/10/2026.
// Path: Engine/include/Engine/Renderer/Handle.hpp
//

#pragma once

#include <cstdint>
#include <functional>

#include <Engine/Utils/HashCombine.hpp>

namespace GEF::Renderer
{
    template <typename Tag>
    struct Handle
    {
        uint32_t index = UINT32_MAX;
        uint32_t generation = 0;

        friend bool operator==(Handle, Handle) = default;

        [[nodiscard]] bool IsValid() const
        {
            return index != UINT32_MAX;
        }
    };

    // Empty struct, used to diff different handle types
    struct BufferTag;
    struct TextureTag;
    struct VertexArrayTag;
    struct ShaderTag;
    struct PipelineTag;

    using BufferHandle = Handle<BufferTag>;
    using TextureHandle = Handle<TextureTag>;
    using VertexArrayHandle = Handle<VertexArrayTag>;
    using ShaderHandle = Handle<ShaderTag>;
    using PipelineHandle = Handle<PipelineTag>;
} // namespace GEF::Renderer

template <typename Tag>
struct std::hash<GEF::Renderer::Handle<Tag>>
{
    std::size_t
    operator()(const GEF::Renderer::Handle<Tag>& handle) const noexcept
    {
        return GEF::Utils::HashAll(handle.index, handle.generation);
    }
};