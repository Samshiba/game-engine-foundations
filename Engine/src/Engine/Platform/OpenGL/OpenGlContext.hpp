//
// Created by genin on 02/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGlContext.hpp
//

#pragma once

#include <Engine/Renderer/GraphicsContext.hpp>

struct GLFWwindow;

namespace GEF::Platform
{
    class OpenGLContext : public Renderer::GraphicsContext
    {
    public:
        OpenGLContext(GLFWwindow* windowHandle);

        void Init() override;
        void SwapBuffers() override;

    private:
        GLFWwindow* windowHandle_;
    };
}