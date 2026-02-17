//
// Created by genin on 09/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGlIndexBuffer.hpp
//

#pragma once

#include <Engine/Renderer/IndexBuffer.hpp>

namespace GEF::Platform
{
    class OpenGlIndexBuffer : public Renderer::IndexBuffer
    {
    public:
        ~OpenGlIndexBuffer() override;
        OpenGlIndexBuffer(uint32_t* indices, uint32_t count);

        void Bind() const override;
        void Unbind() const override;

        [[nodiscard]] uint32_t GetCount() const override;

    private:
        unsigned int id_;
        uint32_t count_;
    };
}