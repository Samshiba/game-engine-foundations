//
// Created by genin on 22/11/2026.
// Path: Engine/include/Engine/Renderer/Layout/CommonLayout.hpp
//

#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include <Engine/Renderer/Buffer.hpp>

namespace GEF::Renderer::Layout
{
    // ---------- Uniform ----------
    // std140 mirrors: vec4 / mat4 only
    enum class CommonUniformBinding : uint32_t
    {
        View = 0,
        Count
    };

    struct ViewUniforms // one buffer per pass (camera, light)
    {
        glm::mat4 viewProjection;
        glm::vec4 cameraPosition; // xyz, w unused
    };
    static_assert(sizeof(ViewUniforms) == 80);

    // ---------- Shader Storage ----------
    // std430
    enum class CommonShaderStorageBinding : uint32_t
    {
        Count = 0,
    };

    // ---------- Texture ----------
    enum class CommonTextureBinding : uint32_t
    {
        Count = 0,
    };

    // ---------- Instance Attributes ----------
    struct TransformInstance
    {
        glm::mat4 model;

        static BufferLayout Layout()
        {
            return { { ShaderDataType::Mat4, "a_Model" } };
        }
    };
} // namespace GEF::Renderer::Layout