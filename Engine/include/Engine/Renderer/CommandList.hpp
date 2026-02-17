//
// Created by genin on 16/02/2026.
// Path: Engine/include/Engine/Renderer/CommandList.hpp
//

#pragma once

#include "VertexArray.hpp"

namespace GEF::Renderer
{
    class CommandList
    {
    public:
        virtual ~CommandList() = default;

        virtual void SetClearColor(float r, float g, float b, float a) = 0;
        virtual void Clear() = 0;

        virtual void BindVertexArray(
            const std::shared_ptr<VertexArray>& vertexArray) = 0;
        virtual void DrawIndexed(uint32_t indexCount) = 0;
    };
}