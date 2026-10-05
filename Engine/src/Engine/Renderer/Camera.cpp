//
// Created by genin on 05/10/2026.
// Path: Engine/src/Engine/Renderer/Camera.cpp
//

#include <Engine/Renderer/Camera.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace GEF::Renderer
{
    glm::vec3 Camera::GetForward() const
    {
        glm::vec3 direction;

        auto rYaw = glm::radians(yaw);
        auto rPitch = glm::radians(pitch);

        direction.x = glm::cos(rYaw) * glm::cos(rPitch);
        direction.y = glm::sin(rPitch);
        direction.z = glm::sin(rYaw) * glm::cos(rPitch);

        return direction;
    }

    glm::vec3 Camera::GetRight() const
    {
        return { -glm::sin(glm::radians(yaw)), 0,
                 glm::cos(glm::radians(yaw)) };
    }

    glm::mat4 Camera::GetView() const
    {
        return glm::lookAt(position, position + GetForward(),
                           glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 Camera::GetProjection() const
    {
        return glm::perspective(glm::radians(fovY), aspectRatio, nearPlane,
                                farPlane);
    }

    glm::mat4 Camera::GetViewProjection() const
    {
        return GetProjection() * GetView();
    }
}