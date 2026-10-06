//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Renderer/LightingData.hpp
//

#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace GEF::Renderer
{
    // The "binding = N" of each uniform block in lit.vert / lit.frag
    namespace UniformBinding
    {
        constexpr uint32_t Frame = 0;
        constexpr uint32_t Object = 1;
        constexpr uint32_t Light = 2;
        constexpr uint32_t Material = 3;
    }

    // std140 mirrors: vec4 / mat4 only

    struct FrameUniforms // binding 0: once per frame
    {
        glm::mat4 viewProjection;
        glm::vec4 cameraPosition; // xyz, w unused
    };

    static_assert(sizeof(FrameUniforms) == 80);

    struct ObjectUniforms // binding 1: once per object
    {
        glm::mat4 model;
        glm::mat4 normalMatrix;
        // a mat3 in std140 is 3 padded vec4 columns, mat3() of it in the shader
    };

    static_assert(sizeof(ObjectUniforms) == 128);

    struct LightUniforms // binding 2: once per frame
    {
        glm::vec4 direction; // xyz = direction the light travels, w unused
        glm::vec4 color;
        glm::vec4 ambient; // rgb = ambient color, a unused
    };

    static_assert(sizeof(LightUniforms) == 48);

    struct MaterialUniforms // binding 3: once per object (or per material)
    {
        glm::vec4 albedo; // rgb = base color (linear), a unused
        glm::vec4 params; // x = shininess, y = specular strength
    };

    static_assert(sizeof(MaterialUniforms) == 32);
}