//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Scene/Components.hpp
//

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Engine/Renderer/Mesh.hpp>

namespace GEF::Scene
{
    struct Transform
    {
        glm::vec3 position{ 0.0f };
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f };

        [[nodiscard]] glm::mat4 GetMatrix() const; // T * R * S
        [[nodiscard]] glm::mat3 GetNormalMatrix() const; // R * S^-1
    };

    inline glm::mat4 Transform::GetMatrix() const
    {
        return glm::translate(glm::mat4(1.0f), position)
            * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
    }

    inline glm::mat3 Transform::GetNormalMatrix() const
    {
        return glm::mat3_cast(rotation)
            * glm::mat3(glm::scale(glm::mat4(1), 1.0f / scale));
    }

    struct Material
    {
        glm::vec3 albedo{ 0.8f };
        // sRGB, as picked by eye: the renderer converts to linear
        float shininess = 32.0f;
        float specularStrength = 0.5f;
    };

    struct MeshRenderer
    {
        Renderer::Mesh mesh{};
        Material material{};
    };

    struct DirectionalLight
    {
        glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
        glm::vec3 color{ 1.0f };
        float intensity = 1.0f;
        glm::vec3 ambient{ 0.1f };
    };
} // namespace GEF::Scene