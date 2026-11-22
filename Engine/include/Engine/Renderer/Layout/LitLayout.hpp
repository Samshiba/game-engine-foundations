//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Renderer/Layout/LitLayout.hpp
//

#pragma once

#include "CommonLayout.hpp"

namespace GEF::Renderer::Layout
{
    // ---------- Uniform ----------
    // std140 mirrors: vec4 / mat4 only
    enum class LitUniformBinding : uint32_t
    {
        Shadow = static_cast<uint32_t>(CommonUniformBinding::Count),
        Environment,
        Count
    };

    struct ShadowUniforms
    {
        glm::mat4 lightViewProjection;
        glm::vec4
            settings; // x = bias, y = normal bias, z = texelSize, w = unused
    };
    static_assert(sizeof(ShadowUniforms) == 80);

    struct EnvironmentUniforms
    {
        glm::vec4 ambient; // rgb = ambient color, a unused
        glm::uvec4 flags; // x = debug view mode, yzw unused
    };
    static_assert(sizeof(EnvironmentUniforms) == 32);

    // ---------- Shader Storage ----------
    // std430
    enum class LitShaderStorageBinding : uint32_t
    {
        DirectionalLight =
            static_cast<uint32_t>(CommonShaderStorageBinding::Count),
        PointLight,
        SpotLight,
        Count
    };

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

    // ---------- Texture ----------
    enum class LitTextureBinding : uint32_t
    {
        Albedo = static_cast<uint32_t>(CommonTextureBinding::Count),
        Normal,
        ShadowMap,
        // Roughness,
        // Metallic,
        // Emissive,
        // AO,
        // Height,
        Count
    };

    // ---------- Instance Attributes ----------
    struct ShadingInstance
    {
        TransformInstance transform;
        glm::mat3 normalMatrix;
        glm::vec3 albedo;
        glm::vec2 params;

        static BufferLayout Layout()
        {
            BufferLayout layout = TransformInstance::Layout();
            layout.Append({ { ShaderDataType::Mat3, "a_NormalMatrix" },
                            { ShaderDataType::Float3, "a_Albedo" },
                            { ShaderDataType::Float2, "a_Params" } });
            return layout;
        }
    };
    static_assert(offsetof(ShadingInstance, transform) == 0);
    static_assert(sizeof(ShadingInstance) == 120);
} // namespace GEF::Renderer::Layout