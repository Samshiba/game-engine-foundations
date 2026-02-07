//
// Created by genin on 07/02/2026.
//

#pragma once

#include <cstdint>

namespace GEF::Renderer
{
    enum class RendererBackend : uint8_t
    {
        None,
        OpenGL,
        Vulkan,
        DirectX11,
        DirectX12,
        Metal
    };
}