//
// Path: tests/Platform/OpenGL/OpenGLShader.test.cpp
//

#include <doctest.h>

#include "GLTestContext.hpp"

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <string_view>

using namespace GEF;

namespace
{
    constexpr std::string_view VertexSource = R"(
        #version 450 core
        layout (location = 0) in vec3 a_Position;
        out vec3 v_Color;
        void main()
        {
            v_Color = a_Position;
            gl_Position = vec4(a_Position, 1.0);
        })";

    constexpr std::string_view FragmentSource = R"(
        #version 450 core
        in vec3 v_Color;
        out vec4 o_Color;
        void main()
        {
            o_Color = vec4(v_Color, 1.0);
        })";

    GLint ProgramParam(GLuint program, GLenum pname)
    {
        GLint value = 0;
        glGetProgramiv(program, pname, &value);
        return value;
    }
}

GL_TEST_CASE("CreateShader links a valid program")
{
    Platform::OpenGLGraphicsDevice device;

    const Renderer::ShaderHandle handle =
        device.CreateShader({ VertexSource, FragmentSource });

    REQUIRE(handle.IsValid());
    const Platform::GLShader* shader = device.GetShader(handle);
    REQUIRE(shader != nullptr);
    CHECK(glIsProgram(shader->program) == GL_TRUE);
    CHECK(ProgramParam(shader->program, GL_LINK_STATUS) == GL_TRUE);

    device.DestroyShader(handle);
}

GL_TEST_CASE("CreateShader reads exactly the string_view, not up to a '\\0'")
{
    Platform::OpenGLGraphicsDevice device;

    // A view into a bigger string: nothing guarantees a '\0' right after it
    const std::string vertexFile = std::string(VertexSource) + "garbage";
    const std::string fragmentFile = std::string(FragmentSource) + "garbage";
    const std::string_view vertexView(vertexFile.data(), VertexSource.size());
    const std::string_view fragmentView(fragmentFile.data(),
                                        FragmentSource.size());

    const Renderer::ShaderHandle handle =
        device.CreateShader({ vertexView, fragmentView });

    REQUIRE(handle.IsValid());
    CHECK(ProgramParam(device.GetShader(handle)->program, GL_LINK_STATUS) ==
          GL_TRUE);

    device.DestroyShader(handle);
}

GL_TEST_CASE("A shader that fails to compile gives an invalid handle")
{
    Platform::OpenGLGraphicsDevice device;

    const Renderer::ShaderHandle handle = device.CreateShader(
        { "#version 450 core\nvoid main() { this does not compile }",
          FragmentSource });

    CHECK_FALSE(handle.IsValid());
    CHECK(device.GetShader(handle) == nullptr);
}

GL_TEST_CASE("A shader that fails to link gives an invalid handle")
{
    Platform::OpenGLGraphicsDevice device;

    // Both stages compile (main() is only required at link time), but the
    // fragment stage has no entry point: the link fails on every driver.
    // (Unmatched varyings are not reliable: some drivers accept them.)
    const Renderer::ShaderHandle handle = device.CreateShader({
        VertexSource,
        R"(#version 450 core
           out vec4 o_Color;
           void NotMain() { o_Color = vec4(1.0); })",
    });

    CHECK_FALSE(handle.IsValid());
    CHECK(device.GetShader(handle) == nullptr);
}

GL_TEST_CASE("DestroyShader deletes the program and invalidates the handle")
{
    Platform::OpenGLGraphicsDevice device;

    const Renderer::ShaderHandle handle =
        device.CreateShader({ VertexSource, FragmentSource });
    const GLuint program = device.GetShader(handle)->program;

    device.DestroyShader(handle);

    CHECK(glIsProgram(program) == GL_FALSE);
    CHECK(device.GetShader(handle) == nullptr);
}

GL_TEST_CASE("The device frees leaked shaders when destroyed")
{
    GLuint program = 0;
    {
        Platform::OpenGLGraphicsDevice device;
        const Renderer::ShaderHandle handle =
            device.CreateShader({ VertexSource, FragmentSource });
        program = device.GetShader(handle)->program;
    }

    CHECK(glIsProgram(program) == GL_FALSE);
}

// --- std140 ----------------------------------------------------------------
// The CPU writes a struct into a uniform buffer, the GPU reads it with the
// std140 rules: both sides must agree on every offset. These tests ask the
// driver for the real layout and compare it with the C++ structs.

namespace
{
    constexpr std::string_view UniformBlockVertexSource = R"(
        #version 450 core
        layout (location = 0) in vec3 a_Position;

        layout (std140, binding = 3) uniform Frame
        {
            mat4 u_Transform;
            vec4 u_Tint;
        };

        layout (std140, binding = 4) uniform Light
        {
            vec3 u_Direction;
            vec3 u_Color;
        };

        out vec4 v_Color;
        void main()
        {
            v_Color = u_Tint * vec4(u_Color * dot(u_Direction, a_Position), 1.0);
            gl_Position = u_Transform * vec4(a_Position, 1.0);
        })";

    constexpr std::string_view UniformBlockFragmentSource = R"(
        #version 450 core
        in vec4 v_Color;
        out vec4 o_Color;
        void main() { o_Color = v_Color; })";

    // Safe: only 16-byte members, so std140 adds no padding
    struct FrameData
    {
        glm::mat4 transform;
        glm::vec4 tint;
    };

    // The trap: in std140 a vec3 is aligned on 16 bytes, in C++ on 4
    struct NaiveLightData
    {
        glm::vec3 direction;
        glm::vec3 color;
    };

    // The fix: explicit padding (or simply vec4 everywhere)
    struct LightData
    {
        glm::vec3 direction;
        float padding0;
        glm::vec3 color;
        float padding1;
    };

    GLint BlockParam(GLuint program, const char* block, GLenum pname)
    {
        const GLuint index = glGetUniformBlockIndex(program, block);
        GLint value = -1;
        glGetActiveUniformBlockiv(program, index, pname, &value);
        return value;
    }

    GLint UniformOffset(GLuint program, const char* name)
    {
        GLuint index = GL_INVALID_INDEX;
        glGetUniformIndices(program, 1, &name, &index);
        GLint offset = -1;
        glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_OFFSET, &offset);
        return offset;
    }
}

GL_TEST_CASE("std140: uniform blocks keep their binding and match the C++ layout")
{
    Platform::OpenGLGraphicsDevice device;
    const Renderer::ShaderHandle handle = device.CreateShader(
        { UniformBlockVertexSource, UniformBlockFragmentSource });
    REQUIRE(handle.IsValid());
    const GLuint program = device.GetShader(handle)->program;

    SUBCASE("binding = N in GLSL is the slot BindUniformBuffer will use")
    {
        CHECK(BlockParam(program, "Frame", GL_UNIFORM_BLOCK_BINDING) == 3);
        CHECK(BlockParam(program, "Light", GL_UNIFORM_BLOCK_BINDING) == 4);
    }

    SUBCASE("mat4 + vec4: the C++ struct matches as is")
    {
        CHECK(BlockParam(program, "Frame", GL_UNIFORM_BLOCK_DATA_SIZE) ==
              static_cast<GLint>(sizeof(FrameData)));
        CHECK(UniformOffset(program, "u_Tint") ==
              static_cast<GLint>(offsetof(FrameData, tint)));
    }

    SUBCASE("vec3 + vec3: the naive C++ struct is off by 4 bytes")
    {
        CHECK(UniformOffset(program, "u_Color") == 16);
        CHECK(offsetof(NaiveLightData, color) == 12); // the silent bug
        CHECK(UniformOffset(program, "u_Color") ==
              static_cast<GLint>(offsetof(LightData, color))); // the fix
    }

    device.DestroyShader(handle);
}
