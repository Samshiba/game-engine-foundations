//
// Created by genin on 02/02/2026.
// Path: Engine/src/Engine/Platform/OpenGL/OpenGlContext.cpp
//

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "OpenGlContext.hpp"

namespace GEF::Platform
{
#ifdef GEF_DEBUG_BUILD
    namespace
    {
        // Forwards driver messages (errors, perf warnings) to the engine log
        void APIENTRY OpenGLDebugCallback([[maybe_unused]] GLenum source,
                                          [[maybe_unused]] GLenum type,
                                          [[maybe_unused]] GLuint id,
                                          GLenum severity,
                                          [[maybe_unused]] GLsizei length,
                                          const GLchar* message,
                                          [[maybe_unused]] const void* user)
        {
            switch (severity)
            {
            case GL_DEBUG_SEVERITY_HIGH:
                GEF_ENGINE_ERROR("OpenGL: {}", message);
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
            case GL_DEBUG_SEVERITY_LOW:
                GEF_ENGINE_WARN("OpenGL: {}", message);
                break;
            default:
                break; // GL_DEBUG_SEVERITY_NOTIFICATION is too verbose
            }
        }
    }
#endif

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

        // The backend relies on DSA, so 4.5
        GEF_CORE_ASSERT(GLVersion.major > 4 ||
                        (GLVersion.major == 4 && GLVersion.minor >= 5),
                        "OpenGL 4.5 or newer is required");

#ifdef GEF_DEBUG_BUILD
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OpenGLDebugCallback, nullptr);
#endif
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(windowHandle_);
    }
}