//
// Created by genin on 02/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGlContext.cpp
//

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "OpenGlContext.hpp"

namespace GEF::Platform
{
    OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
        : windowHandle_(windowHandle)
    {
        GEF_CORE_ASSERT(windowHandle != nullptr, "GLFW window handle is null");
    }

    void OpenGLContext::Init()
    {
        glfwMakeContextCurrent(windowHandle_);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            GEF_ENGINE_ERROR("Failed to initialize GLAD");
            return;
        }

        GEF_ENGINE_INFO("OpenGL Vendor : {}",
                        reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
        GEF_ENGINE_INFO("OpenGL Renderer : {}",
                        reinterpret_cast<const char*>(glGetString(GL_RENDERER)
                        ));
        GEF_ENGINE_INFO("OpenGL Version : {}",
                        reinterpret_cast<const char*>(glGetString(GL_VERSION)));

        // TODO ASSERT OPENGL VERSION
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(windowHandle_);
    }
}