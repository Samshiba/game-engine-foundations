//
// Created by genin on 05/10/2026.
// Path: Application/src/FreeFlyController.cpp
//

#include "FreeFlyController.hpp"

#include <Engine/Core/Window.hpp>

#include <algorithm>

void FreeFlyController::Update(GEF::Renderer::Camera& camera,
                               const GEF::Input& input, GEF::Window& window,
                               float dt)
{
    if (input.IsMouseButtonJustPressed(GEF::Key::MouseCode::MOUSE_BUTTON_RIGHT))
    {
        capturing_ = true;
        window.SetCursorMode(GEF::Window::CursorMode::DISABLED);
    }
    else if (input.IsMouseButtonJustReleased(
        GEF::Key::MouseCode::MOUSE_BUTTON_RIGHT))
    {
        capturing_ = false;
        window.SetCursorMode(GEF::Window::CursorMode::NORMAL);
    }

    if (capturing_)
    {
        auto [dx, dy] = input.GetMouseDelta();
        camera.yaw += static_cast<float>(dx) * sensitivity;
        camera.pitch -= static_cast<float>(dy) * sensitivity;
        camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);

        const auto shift = input.
            IsKeyPressed(GEF::Key::KeyCode::KEY_LEFT_SHIFT);

        glm::vec3 direction(0.0f);

        if (input.IsKeyPressed(GEF::Key::KeyCode::KEY_W))
            direction += camera.GetForward();
        if (input.IsKeyPressed(GEF::Key::KeyCode::KEY_S))
            direction -= camera.GetForward();
        if (input.IsKeyPressed(GEF::Key::KeyCode::KEY_D))
            direction += camera.GetRight();
        if (input.IsKeyPressed(GEF::Key::KeyCode::KEY_A))
            direction -= camera.GetRight();
        if (input.IsKeyPressed(GEF::Key::KeyCode::KEY_SPACE))
            direction += glm::vec3(0.0f, 1.0f, 0.0f);
        if (input.IsKeyPressed(GEF::Key::KeyCode::KEY_LEFT_CONTROL))
            direction -= glm::vec3(0.0f, 1.0f, 0.0f);

        if (glm::length(direction) > 0.0f)
        {
            const float speed = moveSpeed * (shift ? fastMultiplier : 1.0f);
            camera.position += glm::normalize(direction) * speed * dt;
        }
    }
}
