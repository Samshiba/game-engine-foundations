//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Renderer/Layout/LitShaderLayout.hpp
//

#pragma once

#include <cstdint>

#include <glm/glm.hpp>

// ---------- Uniform ----------
namespace GEF::Renderer::Layout
{
    // The "binding = N" of each uniform block in lit.vert / lit.frag
    namespace UniformBinding
    {
        constexpr uint32_t Frame = 0;
        constexpr uint32_t Environment = 1;
    } // namespace UniformBinding

    // std140 mirrors: vec4 / mat4 only

    struct FrameUniforms // binding 0: once per frame
    {
        glm::mat4 viewProjection;
        glm::vec4 cameraPosition; // xyz, w unused
    };
    static_assert(sizeof(FrameUniforms) == 80);

    struct EnvironmentUniforms // binding 1: once per frame
    {
        glm::vec4 ambient; // rgb = ambient color, a unused
    };
    static_assert(sizeof(EnvironmentUniforms) == 16);
} // namespace GEF::Renderer::Layout

// ---------- Shader Storage ----------
namespace GEF::Renderer::Layout
{
    // std430
    namespace ShaderStorageBinding
    {
        constexpr uint32_t DirectionalLight = 0;
        constexpr uint32_t PointLight = 1;
        constexpr uint32_t SpotLight = 2;
    } // namespace ShaderStorageBinding

    // Each light SSBO = header, then the array. In std430 the array of
    // vec4 structs starts on a multiple of 16, so the header is padded to 16.
    struct LightListHeader
    {
        uint32_t count;
        uint32_t pad[3];
    };
    static_assert(sizeof(LightListHeader) == 16);

    // Array elements: vec4 only, size multiple of 16 (= std430 array stride)

    struct GpuDirectionalLight
    {
        glm::vec4 direction; // xyz = direction the light travels, w unused
        glm::vec4 color; // rgb = color * intensity, a unused
    };
    static_assert(sizeof(GpuDirectionalLight) == 32);

    struct GpuPointLight
    {
        glm::vec4 positionRange; // xyz = world position, w = range
        glm::vec4 color;
    };
    static_assert(sizeof(GpuPointLight) == 32);

    struct GpuSpotLight
    {
        glm::vec4 positionRange;
        glm::vec4 direction;
        glm::vec4 color;
        glm::vec4 coneCos; // x = cos(inner), y = cos(outer), zw unused
    };
    static_assert(sizeof(GpuSpotLight) == 64);
} // namespace GEF::Renderer::Layout