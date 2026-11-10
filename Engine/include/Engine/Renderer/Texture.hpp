//
// Created by genin on 10/11/2026.
// Path: Engine/include/Engine/Renderer/Texture.hpp
//

#pragma once

#include "GraphicsDevice.hpp"
#include "Handle.hpp"

namespace GEF::Renderer
{
    struct ImageData // CPU side
    {
        uint32_t width;
        uint32_t height;
        std::vector<uint8_t> pixels;
    };

    using Texture = TextureHandle; // GPU side

    Texture UploadTexture(GraphicsDevice& device, const ImageData& data,
                          TextureFormat format);
    void DestroyTexture(GraphicsDevice& device, const Texture& texture);
} // namespace GEF::Renderer