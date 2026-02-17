//
// Created by genin on 08/02/2026.
// Path: Engine/include/Engine/Renderer/VertexArray.hpp
//

#pragma once

#include "IndexBuffer.hpp"
#include "VertexBuffer.hpp"

namespace GEF::Renderer
{
    class VertexArray
    {
    public:
        virtual ~VertexArray() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void AddVertexBuffer(
            const std::shared_ptr<VertexBuffer>& vertexBuffer) = 0;
        virtual void SetIndexBuffer(
            const std::shared_ptr<IndexBuffer>& indexBuffer) = 0;

        [[nodiscard]] virtual const std::vector<std::shared_ptr<VertexBuffer> >&
        GetVertexBuffers() const = 0;
        [[nodiscard]] virtual const std::shared_ptr<IndexBuffer>&
        GetIndexBuffer() const = 0;
    };
}