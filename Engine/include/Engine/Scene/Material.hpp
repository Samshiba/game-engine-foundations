//
// Created by genin on 10/11/2026.
// Path: Engine/include/Engine/Scene/Material.hpp
//

#pragma once

#include <glm/glm.hpp>

#include <Engine/Renderer/Handle.hpp>

#include "Engine/Renderer/Texture.hpp"

namespace GEF::Scene
{
    struct Material
    {
        glm::vec3 albedo{ 0.8f };
        // sRGB, as picked by eye: the renderer converts to linear
        float shininess = 32.0f;
        float specularStrength = 0.5f;

        Renderer::Texture albedoMap{};
        Renderer::Texture normalMap{};
    };
} // namespace GEF::Scene