//
// Created by genin on 08/02/2026.
// Path: Engine/include/Engine/Renderer/VertexBuffer.hpp
//

#pragma once

#include "Buffer.hpp"

namespace GEF::Renderer
{
    class VertexBuffer
    {
    public:
        virtual ~VertexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void SetLayout(const BufferLayout& layout) = 0;
        [[nodiscard]] virtual const BufferLayout& GetLayout() const = 0;
    };
}