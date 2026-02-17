//
// Created by genin on 08/02/2026.
// Path: Engine/include/Engine/Renderer/IndexBuffer.hpp
//

#pragma once

#include "Buffer.hpp"

namespace GEF::Renderer
{
    class IndexBuffer
    {
    public:
        virtual ~IndexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        [[nodiscard]] virtual uint32_t GetCount() const = 0;
    };
}