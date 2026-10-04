//
// Created by genin on 04/10/2026.
// Path: tests/Platform/OpenGL/OpenGLGraphicsDevice.test.cpp
//

#include <doctest.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>

#include <array>
#include <cstdint>

using namespace GEF;

namespace
{
    // One hidden window with an OpenGL 4.5 context, shared by every test and
    // destroyed at exit (after the devices, which are test-local).
    // Machines without a 4.5 driver (e.g. GPU-less CI runners) skip the tests.
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

    // Returns from the test case when no context can be created
#define REQUIRE_GL_CONTEXT()                                                   \
    if (!GLTestContext::Available())                                           \
    {                                                                          \
        MESSAGE("No OpenGL 4.5 context available: test skipped");             \
        return;                                                                \
    }

    constexpr std::array<float, 6> Vertices = { 0.f, 1.f, 2.f, 3.f, 4.f, 5.f };

    Renderer::BufferDesc VertexDesc()
    {
        return { Renderer::BufferType::Vertex,
                 static_cast<uint32_t>(sizeof(Vertices)), Vertices.data() };
    }
}

TEST_CASE("CreateBuffer returns a valid handle to a GL buffer")
{
    REQUIRE_GL_CONTEXT();
    Platform::OpenGLGraphicsDevice device;

    const Renderer::BufferHandle handle = device.CreateBuffer(VertexDesc());

    const Platform::GLBuffer* buffer = device.GetBuffer(handle);
    REQUIRE(buffer != nullptr);
    CHECK(glIsBuffer(buffer->id) == GL_TRUE);
    CHECK(buffer->size == sizeof(Vertices));
    CHECK(buffer->type == Renderer::BufferType::Vertex);

    device.DestroyBuffer(handle);
}

TEST_CASE("CreateBuffer uploads the data to the GPU")
{
    REQUIRE_GL_CONTEXT();
    Platform::OpenGLGraphicsDevice device;

    const Renderer::BufferHandle handle = device.CreateBuffer(VertexDesc());

    GLint gpuSize = 0;
    std::array<float, 6> readBack{};
    const GLuint id = device.GetBuffer(handle)->id;
    glGetNamedBufferParameteriv(id, GL_BUFFER_SIZE, &gpuSize);
    glGetNamedBufferSubData(id, 0, sizeof(readBack), readBack.data());

    CHECK(gpuSize == static_cast<GLint>(sizeof(Vertices)));
    CHECK(readBack == Vertices);

    device.DestroyBuffer(handle);
}

TEST_CASE("CreateBuffer accepts no data (allocation only)")
{
    REQUIRE_GL_CONTEXT();
    Platform::OpenGLGraphicsDevice device;

    const Renderer::BufferHandle handle =
        device.CreateBuffer({ Renderer::BufferType::Vertex, 256, nullptr });

    GLint gpuSize = 0;
    glGetNamedBufferParameteriv(device.GetBuffer(handle)->id, GL_BUFFER_SIZE,
                                &gpuSize);
    CHECK(gpuSize == 256);

    device.DestroyBuffer(handle);
}

TEST_CASE("DestroyBuffer deletes the GL buffer and invalidates the handle")
{
    REQUIRE_GL_CONTEXT();
    Platform::OpenGLGraphicsDevice device;

    const Renderer::BufferHandle handle = device.CreateBuffer(VertexDesc());
    const GLuint id = device.GetBuffer(handle)->id;

    device.DestroyBuffer(handle);

    CHECK(glIsBuffer(id) == GL_FALSE);
    CHECK(device.GetBuffer(handle) == nullptr);
}

TEST_CASE("A destroyed buffer's slot is reused with a new generation")
{
    REQUIRE_GL_CONTEXT();
    Platform::OpenGLGraphicsDevice device;

    const Renderer::BufferHandle first = device.CreateBuffer(VertexDesc());
    device.DestroyBuffer(first);
    const Renderer::BufferHandle second = device.CreateBuffer(VertexDesc());

    CHECK(second.index == first.index);
    CHECK(second.generation == first.generation + 1);
    CHECK(device.GetBuffer(first) == nullptr);
    CHECK(device.GetBuffer(second) != nullptr);

    device.DestroyBuffer(second);
}

TEST_CASE("The device frees leaked buffers when destroyed")
{
    REQUIRE_GL_CONTEXT();
    GLuint id = 0;
    {
        Platform::OpenGLGraphicsDevice device;
        const Renderer::BufferHandle handle = device.CreateBuffer(VertexDesc());
        id = device.GetBuffer(handle)->id;
        // no DestroyBuffer: the device must clean up (and log the leak)
    }

    CHECK(glIsBuffer(id) == GL_FALSE);
}
