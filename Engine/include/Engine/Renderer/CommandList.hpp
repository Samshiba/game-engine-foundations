//
// Created by genin on 16/02/2026.
// Path: Engine/include/Engine/Renderer/CommandList.hpp
//

#pragma once

#include <cstdint>
#include <memory>

#include "Handle.hpp"

namespace GEF::Renderer
{
    class CommandList
    {
    public:
        virtual ~CommandList() = default;

        virtual void SetClearColor(float r, float g, float b, float a) = 0;
        virtual void Clear() = 0;

        virtual void BindVertexArray(VertexArrayHandle handle) = 0;
        virtual void BindPipeline(PipelineHandle handle) = 0;
        virtual void BindUniformBuffer(uint32_t bindingPoint,
                                       BufferHandle handle) = 0;

        virtual void DrawIndexed(uint32_t indexCount) = 0;
    };
}