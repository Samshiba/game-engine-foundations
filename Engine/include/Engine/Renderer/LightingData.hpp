//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Renderer/LightingData.hpp
//

#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace GEF::Renderer
{
    // The "binding = N" of each uniform block in lit.vert / lit.frag
    namespace UniformBinding
    {
        constexpr uint32_t Frame = 0;
        constexpr uint32_t Light = 1;
    } // namespace UniformBinding

    constexpr uint32_t MaxUniformBindings = 2;

    // std140 mirrors: vec4 / mat4 only

    struct FrameUniforms // binding 0: once per frame
    {
        glm::mat4 viewProjection;
        glm::vec4 cameraPosition; // xyz, w unused
    };

    static_assert(sizeof(FrameUniforms) == 80);

    struct LightUniforms // binding 2: once per frame
    {
        glm::vec4 direction; // xyz = direction the light travels, w unused
        glm::vec4 color;
        glm::vec4 ambient; // rgb = ambient color, a unused
    };

    static_assert(sizeof(LightUniforms) == 48);
} // namespace GEF::Renderer