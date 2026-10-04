//
// Path: tests/Platform/OpenGL/GLTestContext.hpp
//

#pragma once

#include <doctest.h>

// glad must come before GLFW, which would otherwise pull the system gl.h
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace GEF::Tests
{
    // One hidden window with an OpenGL 4.5 context, shared by every test and
    // destroyed at exit (after the devices, which are test-local).
    class GLTestContext
    {
    public:
        static bool Available()
        {
            static GLTestContext context;
            return context.available_;
        }

        GLTestContext(const GLTestContext&) = delete;
        GLTestContext& operator=(const GLTestContext&) = delete;

    private:
        GLTestContext()
        {
            if (!glfwInit())
                return;

            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

            window_ = glfwCreateWindow(64, 64, "GEF_Tests", nullptr, nullptr);
            if (!window_)
                return;

            glfwMakeContextCurrent(window_);
            available_ = gladLoadGLLoader(
                reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) != 0;
        }

        ~GLTestContext()
        {
            if (window_)
                glfwDestroyWindow(window_);
            glfwTerminate();
        }

        GLFWwindow* window_ = nullptr;
        bool available_ = false;
    };
}

// A test case that needs an OpenGL 4.5 context. Without one (e.g. GPU-less CI
// runners), it is reported as skipped instead of silently passing.
#define GL_TEST_CASE(name)                                                     \
    TEST_CASE(name * doctest::skip(!GEF::Tests::GLTestContext::Available()))
