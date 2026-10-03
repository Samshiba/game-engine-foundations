//
// Created by genin on 07/02/2026.
// Path: Engine/src/Engine/Core/Input.hpp
//

#pragma once

#include <cstddef>
#include <utility>

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
        static constexpr size_t KeyCount =
            static_cast<size_t>(Key::KeyCode::KEY_LAST) + 1;
        static constexpr size_t MouseButtonCount =
            static_cast<size_t>(Key::MouseCode::MOUSE_BUTTON_LAST) + 1;

        static bool keyData_[KeyCount];
        static bool prevKeyData_[KeyCount];

        static bool mouseButtons_[MouseButtonCount];
        static bool prevMouseButtons_[MouseButtonCount];

        static std::pair<double, double> mousePos_;
        static std::pair<double, double> prevMousePos_;
    };
}