//
// Created by genin on 07/02/2026.
// Path: Engine/include/Engine/Core/Input.hpp
//

#pragma once

#include <cstddef>
#include <utility>

#include "KeyCodes.hpp"

namespace GEF
{
    class Window;

    // Polled input state, owned by the Engine and refreshed once per frame
    class Input
    {
    public:
        void Update(const Window& window);

        [[nodiscard]] bool IsKeyPressed(Key::KeyCode key) const;
        [[nodiscard]] bool IsKeyJustPressed(Key::KeyCode key) const;
        [[nodiscard]] bool IsKeyJustReleased(Key::KeyCode key) const;

        [[nodiscard]] bool IsMouseButtonPressed(Key::MouseCode button) const;
        [[nodiscard]] bool IsMouseButtonJustPressed(
            Key::MouseCode button) const;
        [[nodiscard]] bool IsMouseButtonJustReleased(
            Key::MouseCode button) const;

        [[nodiscard]] std::pair<double, double> GetMousePosition() const;
        [[nodiscard]] std::pair<double, double> GetMouseDelta() const;

    private:
        // *_LAST is a valid code, so the arrays need one more slot
        static constexpr size_t KeyCount =
            static_cast<size_t>(Key::KeyCode::KEY_LAST) + 1;
        static constexpr size_t MouseButtonCount =
            static_cast<size_t>(Key::MouseCode::MOUSE_BUTTON_LAST) + 1;

        bool keyData_[KeyCount] = {};
        bool prevKeyData_[KeyCount] = {};

        bool mouseButtons_[MouseButtonCount] = {};
        bool prevMouseButtons_[MouseButtonCount] = {};

        std::pair<double, double> mousePos_ = { 0.0, 0.0 };
        std::pair<double, double> prevMousePos_ = { 0.0, 0.0 };
    };
}
