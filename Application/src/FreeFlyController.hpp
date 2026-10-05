//
// Created by genin on 05/10/2026.
// Path: Application/src/FreeFlyController.hpp
//

#pragma once

#include <Engine/Core/Input.hpp>
#include <Engine/Renderer/Camera.hpp>

namespace GEF
{
    class Window;
}

class FreeFlyController
{
public:
    float moveSpeed = 3.0f;
    float fastMultiplier = 4.0f;
    float sensitivity = 0.1f;

    void Update(GEF::Renderer::Camera& camera, const GEF::Input& input,
                GEF::Window& window, float dt);

private:
    bool capturing_ = false;
};
