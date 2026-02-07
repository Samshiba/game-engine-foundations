//
// Created by genin on 07/02/2026.
//

#pragma once

namespace GEF::Events
{
    inline WindowResizeEvent::WindowResizeEvent(int width, int height)
        : width_(width), height_(height)
    {
    }

    inline std::string WindowResizeEvent::ToString() const
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << width_ << ", " << height_;
        return ss.str();
    }

    inline unsigned int WindowResizeEvent::GetWidth() const
    {
        return width_;
    }

    inline unsigned int WindowResizeEvent::GetHeight() const
    {
        return height_;
    }

    inline KeyPressedEvent::KeyPressedEvent(int key, int repeat)
        : key_(key), repeat_(repeat)
    {
    }

    inline std::string KeyPressedEvent::ToString() const
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << key_ << ", "
            << (repeat_ ? "Held" : "Pressed");
        return ss.str();
    }

    inline int KeyPressedEvent::GetKey() const
    {
        return key_;
    }

    inline bool KeyPressedEvent::IsHeld() const
    {
        return repeat_ ? true : false;
    }

    inline KeyReleasedEvent::KeyReleasedEvent(int key)
        : key_(key)
    {
    }

    inline std::string KeyReleasedEvent::ToString() const
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << key_;
        return ss.str();
    }

    inline int KeyReleasedEvent::GetKey() const
    {
        return key_;
    }

    inline MouseButtonPressedEvent::MouseButtonPressedEvent(int button)
        : button_(button)
    {
    }

    inline std::string MouseButtonPressedEvent::ToString() const
    {
        std::stringstream ss;
        ss << "MouseButtonPressedEvent: " << button_;
        return ss.str();
    }

    inline int MouseButtonPressedEvent::GetButton() const
    {
        return button_;
    }

    inline MouseButtonReleasedEvent::MouseButtonReleasedEvent(int button)
        : button_(button)
    {
    }

    inline std::string MouseButtonReleasedEvent::ToString() const
    {
        std::stringstream ss;
        ss << "MouseButtonReleasedEvent: " << button_;
        return ss.str();
    }

    inline int MouseButtonReleasedEvent::GetButton() const
    {
        return button_;
    }

    inline MouseMovedEvent::MouseMovedEvent(double x, double y)
        : x_(x), y_(y)
    {
    }

    inline std::string MouseMovedEvent::ToString() const
    {
        std::stringstream ss;
        ss << "MouseMovedEvent: " << x_ << ", " << y_;
        return ss.str();
    }

    inline double MouseMovedEvent::GetX() const
    {
        return x_;
    }

    inline double MouseMovedEvent::GetY() const
    {
        return y_;
    }

    inline MouseScrolledEvent::MouseScrolledEvent(double xOffset,
                                                  double yOffset)
        : xOffset_(xOffset), yOffset_(yOffset)
    {
    }

    inline std::string MouseScrolledEvent::ToString() const
    {
        std::stringstream ss;
        ss << "MouseScrolledEvent: " << xOffset_ << ", " << yOffset_;
        return ss.str();
    }

    inline double MouseScrolledEvent::GetXOffset() const
    {
        return xOffset_;
    }

    inline double MouseScrolledEvent::GetYOffset() const
    {
        return yOffset_;
    }
}