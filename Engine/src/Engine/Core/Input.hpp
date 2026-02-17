//
// Created by genin on 07/02/2026.
// Path: Engine/src/Engine/Core/Input.hpp
//

#pragma once

#include "KeyCodes.hpp"

namespace GEF
{
    class Input
    {
    public:
        static void Update();

        [[nodiscard]] static bool IsKeyPressed(Key::KeyCode key);
        [[nodiscard]] static bool IsKeyJustPressed(Key::KeyCode key);
        [[nodiscard]] static bool IsKeyJustReleased(Key::KeyCode key);

        [[nodiscard]] static bool IsMouseButtonPressed(Key::MouseCode button);
        [[nodiscard]] static bool IsMouseButtonJustPressed(
            Key::MouseCode button);
        [[nodiscard]] static bool IsMouseButtonJustReleased(
            Key::MouseCode button);

        [[nodiscard]] static std::pair<double, double> GetMousePosition();
        [[nodiscard]] static std::pair<double, double> GetMouseDelta();

    private:
        static bool keyData_[static_cast<size_t>(Key::KeyCode::KEY_LAST)];
        static bool prevKeyData_[static_cast<size_t>(Key::KeyCode::KEY_LAST)];

        static bool mouseButtons_[static_cast<size_t>(
            Key::MouseCode::MOUSE_BUTTON_LAST)];
        static bool prevMouseButtons_[static_cast<size_t>(
            Key::MouseCode::MOUSE_BUTTON_LAST)];

        static std::pair<double, double> mousePos_;
        static std::pair<double, double> prevMousePos_;
    };
}