//
// Created by genin on 07/02/2026.
// Path: Engine/src/Engine/Core/Input.cpp
//

#include <Engine/Core/Application.hpp>
#include <GLFW/glfw3.h>

#include <cstring>

#include "Commons.hpp"
#include "Input.hpp"

namespace GEF
{
    // Helper functions for enum to size_t/int conversion
    namespace
    {
        template <typename E>
        constexpr size_t ToSz(E e)
        {
            return static_cast<size_t>(e);
        }

        template <typename E>
        constexpr int ToInt(E e)
        {
            return static_cast<int>(e);
        }
    }

    // Static member init
    bool Input::keyData_[KeyCount] = { false };
    bool Input::prevKeyData_[KeyCount] = { false };
    bool Input::mouseButtons_[MouseButtonCount] = { false };
    bool Input::prevMouseButtons_[MouseButtonCount] = { false };
    std::pair<double, double> Input::mousePos_ = { 0.0f, 0.0f };
    std::pair<double, double> Input::prevMousePos_ = { 0.0f, 0.0f };

    void Input::Update()
    {
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().
            GetNativeWindow());

        // --- Keyboard ---
        std::memcpy(prevKeyData_, keyData_, sizeof(keyData_));

        for (int i = ToInt(Key::KeyCode::KEY_SPACE);
             i <= ToInt(Key::KeyCode::KEY_LAST); ++i)
        {
            const int state = glfwGetKey(window, i);
            keyData_[i] = (state == GLFW_PRESS || state == GLFW_REPEAT);
        }

        // --- Mouse Buttons ---
        std::memcpy(prevMouseButtons_, mouseButtons_, sizeof(mouseButtons_));

        for (int i = 0; i <= ToInt(Key::MouseCode::MOUSE_BUTTON_LAST); ++i)
        {
            const int state = glfwGetMouseButton(window, i);
            mouseButtons_[i] = (state == GLFW_PRESS);
        }

        // --- Mouse Position ---
        prevMousePos_ = mousePos_;

        double xPos;
        double yPos;
        glfwGetCursorPos(window, &xPos, &yPos);
        mousePos_ = { xPos, yPos };
    }

    bool Input::IsKeyPressed(Key::KeyCode key)
    {
        if (key > Key::KeyCode::KEY_LAST)
        {
            GEF_ENGINE_WARN("Invalid key code: {}", ToSz(key));
            return false;
        }
        return keyData_[ToSz(key)];
    }

    bool Input::IsKeyJustPressed(Key::KeyCode key)
    {
        if (key > Key::KeyCode::KEY_LAST)
        {
            GEF_ENGINE_WARN("Invalid key code: {}", ToSz(key));
            return false;
        }
        return keyData_[ToSz(key)] &&
            !prevKeyData_[ToSz(key)];
    }

    bool Input::IsKeyJustReleased(Key::KeyCode key)
    {
        if (key > Key::KeyCode::KEY_LAST)
        {
            GEF_ENGINE_WARN("Invalid key code: {}", ToSz(key));
            return false;
        }
        return !keyData_[ToSz(key)] &&
            prevKeyData_[ToSz(key)];
    }

    bool Input::IsMouseButtonPressed(Key::MouseCode button)
    {
        if (button > Key::MouseCode::MOUSE_BUTTON_LAST)
        {
            GEF_ENGINE_WARN("Invalid mouse button code: {}",
                            ToSz(button));
            return false;
        }

        return mouseButtons_[ToSz(button)];
    }

    bool Input::IsMouseButtonJustPressed(Key::MouseCode button)
    {
        if (button > Key::MouseCode::MOUSE_BUTTON_LAST)
        {
            GEF_ENGINE_WARN("Invalid mouse button code: {}",
                            ToSz(button));
            return false;
        }

        return mouseButtons_[ToSz(button)] &&
            !prevMouseButtons_[ToSz(button)];
    }

    bool Input::IsMouseButtonJustReleased(Key::MouseCode button)
    {
        if (button > Key::MouseCode::MOUSE_BUTTON_LAST)
        {
            GEF_ENGINE_WARN("Invalid mouse button code: {}",
                            ToSz(button));
            return false;
        }

        return !mouseButtons_[ToSz(button)] &&
            prevMouseButtons_[ToSz(button)];
    }

    std::pair<double, double> Input::GetMousePosition()
    {
        return mousePos_;
    }

    std::pair<double, double> Input::GetMouseDelta()
    {
        return { mousePos_.first - prevMousePos_.first,
                 mousePos_.second - prevMousePos_.second };
    }
}