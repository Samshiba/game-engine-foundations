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

// --- Vertex arrays ---------------------------------------------------------

namespace
{
    GLint AttribParam(GLuint vao, GLuint attrib, GLenum pname)
    {
        GLint value = 0;
        glGetVertexArrayIndexediv(vao, attrib, pname, &value);
        return value;
    }

    // Which binding (buffer slot) an attribute reads from. No DSA query
    // exists for it: the VAO has to be bound.
    GLint AttribBinding(GLuint vao, GLuint attrib)
    {
        GLint value = 0;
        glBindVertexArray(vao);
        glGetVertexAttribiv(attrib, GL_VERTEX_ATTRIB_BINDING, &value);
        glBindVertexArray(0);
        return value;
    }

    struct TriangleBuffers
    {
        Renderer::BufferHandle vertices;
        Renderer::BufferHandle indices;
    };

    TriangleBuffers CreateTriangleBuffers(Platform::OpenGLGraphicsDevice& device)
    {
        static constexpr std::array<uint32_t, 3> Indices = { 0, 1, 2 };
        return { device.CreateBuffer(VertexDesc()),
                 device.CreateBuffer({ Renderer::BufferType::Index,
                                       static_cast<uint32_t>(sizeof(Indices)),
                                       Indices.data() }) };
    }

    // position (Float3) + color (Float3), interleaved in one buffer
    Renderer::BufferLayout PositionColorLayout()
    {
        return { { Renderer::ShaderDataType::Float3, "a_Position" },
                 { Renderer::ShaderDataType::Float3, "a_Color" } };
    }
}

TEST_CASE("CreateVertexArray returns a valid handle to a GL vertex array")
{
    REQUIRE_GL_CONTEXT();
    Platform::OpenGLGraphicsDevice device;
    const TriangleBuffers buffers = CreateTriangleBuffers(device);

    const Renderer::VertexArrayHandle handle = device.CreateVertexArray({
        .vertexBuffers = { { .buffer = buffers.vertices,
                             .layout = PositionColorLayout() } },
        .indexBuffer = buffers.indices,
    });

    const Platform::GLVertexArray* vertexArray = device.GetVertexArray(handle);
    REQUIRE(vertexArray != nullptr);
    CHECK(glIsVertexArray(vertexArray->id) == GL_TRUE);

    device.DestroyVertexArray(handle);
    device.DestroyBuffer(buffers.indices);
    device.DestroyBuffer(buffers.vertices);
}

TEST_CASE("One binding: every attribute reads the same interleaved buffer")
{
    REQUIRE_GL_CONTEXT();
    Platform::OpenGLGraphicsDevice device;
    const TriangleBuffers buffers = CreateTriangleBuffers(device);

    const Renderer::VertexArrayHandle handle = device.CreateVertexArray({
        .vertexBuffers = { { .buffer = buffers.vertices,
                             .layout = PositionColorLayout() } },
        .indexBuffer = buffers.indices,
    });
    const GLuint vao = device.GetVertexArray(handle)->id;

    for (GLuint attrib = 0; attrib < 2; ++attrib)
    {
        CAPTURE(attrib);
        CHECK(AttribParam(vao, attrib, GL_VERTEX_ATTRIB_ARRAY_ENABLED) == GL_TRUE);
        CHECK(AttribParam(vao, attrib, GL_VERTEX_ATTRIB_ARRAY_SIZE) == 3);
        CHECK(AttribParam(vao, attrib, GL_VERTEX_ATTRIB_ARRAY_TYPE) == GL_FLOAT);
        CHECK(AttribParam(vao, attrib, GL_VERTEX_ATTRIB_ARRAY_DIVISOR) == 0);
        CHECK(AttribBinding(vao, attrib) == 0);
    }
    // a_Color starts after the 3 floats of a_Position
    CHECK(AttribParam(vao, 0, GL_VERTEX_ATTRIB_RELATIVE_OFFSET) == 0);
    CHECK(AttribParam(vao, 1, GL_VERTEX_ATTRIB_RELATIVE_OFFSET) == 12);

    // The index buffer is attached to the VAO
    GLint elementBuffer = 0;
    glGetVertexArrayiv(vao, GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementBuffer);
    CHECK(static_cast<GLuint>(elementBuffer) ==
          device.GetBuffer(buffers.indices)->id);

    device.DestroyVertexArray(handle);
    device.DestroyBuffer(buffers.indices);
    device.DestroyBuffer(buffers.vertices);
}

TEST_CASE("Two bindings: per-instance attributes continue the numbering")
{
    REQUIRE_GL_CONTEXT();
    Platform::OpenGLGraphicsDevice device;
    const TriangleBuffers buffers = CreateTriangleBuffers(device);
    // One mat4 (64 bytes) + one vec4 color per instance
    const Renderer::BufferHandle instances = device.CreateBuffer(
        { Renderer::BufferType::Vertex, 2 * 80, nullptr });

    const Renderer::VertexArrayHandle handle = device.CreateVertexArray({
        .vertexBuffers = {
            { .buffer = buffers.vertices, .layout = PositionColorLayout() },
            { .buffer = instances,
              .layout = { { Renderer::ShaderDataType::Mat4, "a_Model" },
                          { Renderer::ShaderDataType::Float4, "a_Tint" } },
              .instanceDivisor = 1 },
        },
        .indexBuffer = buffers.indices,
    });
    const GLuint vao = device.GetVertexArray(handle)->id;

    // Attributes 0-1 come from binding 0 (per vertex)
    CHECK(AttribBinding(vao, 1) == 0);
    CHECK(AttribParam(vao, 1, GL_VERTEX_ATTRIB_ARRAY_DIVISOR) == 0);

    // The mat4 takes attributes 2 to 5, one vec4 column each, 16 bytes apart
    for (GLuint column = 0; column < 4; ++column)
    {
        const GLuint attrib = 2 + column;
        CAPTURE(attrib);
        CHECK(AttribBinding(vao, attrib) == 1);
        CHECK(AttribParam(vao, attrib, GL_VERTEX_ATTRIB_ARRAY_SIZE) == 4);
        CHECK(AttribParam(vao, attrib, GL_VERTEX_ATTRIB_ARRAY_DIVISOR) == 1);
        CHECK(AttribParam(vao, attrib, GL_VERTEX_ATTRIB_RELATIVE_OFFSET) ==
              static_cast<GLint>(16 * column));
    }

    // The tint follows the matrix in the instance buffer
    CHECK(AttribBinding(vao, 6) == 1);
    CHECK(AttribParam(vao, 6, GL_VERTEX_ATTRIB_RELATIVE_OFFSET) == 64);

    device.DestroyVertexArray(handle);
    device.DestroyBuffer(instances);
    device.DestroyBuffer(buffers.indices);
    device.DestroyBuffer(buffers.vertices);
}

TEST_CASE("DestroyVertexArray deletes the GL vertex array and invalidates the handle")
{
    REQUIRE_GL_CONTEXT();
    Platform::OpenGLGraphicsDevice device;
    const TriangleBuffers buffers = CreateTriangleBuffers(device);

    const Renderer::VertexArrayHandle handle = device.CreateVertexArray({
        .vertexBuffers = { { .buffer = buffers.vertices,
                             .layout = PositionColorLayout() } },
        .indexBuffer = buffers.indices,
    });
    const GLuint id = device.GetVertexArray(handle)->id;

    device.DestroyVertexArray(handle);

    CHECK(glIsVertexArray(id) == GL_FALSE);
    CHECK(device.GetVertexArray(handle) == nullptr);

    device.DestroyBuffer(buffers.indices);
    device.DestroyBuffer(buffers.vertices);
}

TEST_CASE("The device frees leaked vertex arrays and their buffers")
{
    REQUIRE_GL_CONTEXT();
    GLuint vao = 0;
    GLuint vbo = 0;
    {
        Platform::OpenGLGraphicsDevice device;
        const TriangleBuffers buffers = CreateTriangleBuffers(device);
        const Renderer::VertexArrayHandle handle = device.CreateVertexArray({
            .vertexBuffers = { { .buffer = buffers.vertices,
                                 .layout = PositionColorLayout() } },
            .indexBuffer = buffers.indices,
        });
        vao = device.GetVertexArray(handle)->id;
        vbo = device.GetBuffer(buffers.vertices)->id;
        // nothing destroyed: the device cleans everything up
    }

    CHECK(glIsVertexArray(vao) == GL_FALSE);
    CHECK(glIsBuffer(vbo) == GL_FALSE);
}
