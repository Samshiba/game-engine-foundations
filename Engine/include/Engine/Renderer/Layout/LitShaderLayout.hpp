//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Renderer/LightingData.hpp
//

#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace GEF::Renderer::Layout
{
    // The "binding = N" of each uniform block in lit.vert / lit.frag
    namespace UniformBinding
    {
        constexpr uint32_t Frame = 0;
        constexpr uint32_t Light = 1;
        constexpr uint32_t Environment = 2;
    } // namespace UniformBinding

    // std140 mirrors: vec4 / mat4 only

    struct FrameUniforms // binding 0: once per frame
    {
        glm::mat4 viewProjection;
        glm::vec4 cameraPosition; // xyz, w unused
    };
    static_assert(sizeof(FrameUniforms) == 80);

    struct LightUniforms // binding 1: once per frame
    {
        glm::vec4 direction; // xyz = direction the light travels, w unused
        glm::vec4 color;
    };
    static_assert(sizeof(LightUniforms) == 32);

    struct EnvironmentUniforms // binding 2: once per frame
    {
        glm::vec4 ambient; // rgb = ambient color, a unused
    };
    static_assert(sizeof(EnvironmentUniforms) == 16);
} // namespace GEF::Renderer::Layout