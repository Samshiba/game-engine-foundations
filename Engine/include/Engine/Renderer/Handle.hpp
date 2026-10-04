//
// Created by genin on 04/10/2026.
// Path: Engine/include/Engine/Renderer/Handle.hpp
//

#pragma once
#include <cstdint>

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

    struct BufferTag;
    struct TextureTag;

    using BufferHandle = Handle<BufferTag>;
    using TextureHandle = Handle<TextureTag>;
}