//
// Created by genin on 05/10/2026.
// Path: Engine/include/Engine/Renderer/Camera.hpp
//

#pragma once

#include <glm/glm.hpp>

namespace GEF::Renderer
{
    struct Camera
    {
        glm::vec3 position{ 0.0f, 0.0f, 3.0f };
        float yaw = -90.0f; // looking down -Z
        float pitch = 0.0f; // clamped to +-89 by the controller
        float fovY = 60.0f; // vertical field of view
        float aspectRatio = 16.0f / 9.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;

        [[nodiscard]] glm::vec3 GetForward() const;
        [[nodiscard]] glm::vec3 GetRight() const;
        [[nodiscard]] glm::mat4 GetView() const;
        [[nodiscard]] glm::mat4 GetProjection() const;
        [[nodiscard]] glm::mat4 GetViewProjection() const;
    };
}