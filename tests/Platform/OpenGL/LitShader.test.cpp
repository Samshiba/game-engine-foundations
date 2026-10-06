//
// Path: tests/Platform/OpenGL/LitShader.test.cpp
//
// The real lit shaders (Application/assets/shaders/lit.*) against the C++
// mirrors in LightingData.hpp. A block whose binding, size or member offset
// drifts from its struct renders garbage without any GL error: this catches
// it before the screen does (e.g. two blocks bound to each other's slot).
//

#include <doctest.h>

#include "GLTestContext.hpp"

#include <Engine/Core/FileSystem.hpp>
#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/LightingData.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

using namespace GEF;
using namespace GEF::Renderer;

namespace
{
    std::optional<std::string> ReadShader(const char* fileName)
    {
        return FileSystem::ReadTextFile(std::filesystem::path(GEF_SOURCE_DIR) /
                                        "Application" / "assets" / "shaders" /
                                        fileName);
    }

    struct BlockInfo
    {
        GLint binding = -1;
        GLint size = -1;
    };

    BlockInfo Block(GLuint program, const char* name)
    {
        BlockInfo info;
        const GLuint index = glGetUniformBlockIndex(program, name);
        if (index == GL_INVALID_INDEX)
            return info;
        glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_BINDING,
                                  &info.binding);
        glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_DATA_SIZE,
                                  &info.size);
        return info;
    }

    GLint Offset(GLuint program, const char* member)
    {
        GLuint index = GL_INVALID_INDEX;
        glGetUniformIndices(program, 1, &member, &index);
        if (index == GL_INVALID_INDEX)
            return -1;
        GLint offset = -1;
        glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_OFFSET, &offset);
        return offset;
    }

    template <typename T>
    GLint SizeOf()
    {
        return static_cast<GLint>(sizeof(T));
    }

    // The lit shader, compiled once per test from the real files
    struct LitShaderFixture
    {
        Platform::OpenGLGraphicsDevice device;
        ShaderHandle shader;
        GLuint program = 0;

        LitShaderFixture()
        {
            const auto vertex = ReadShader("lit.vert");
            const auto fragment = ReadShader("lit.frag");
            REQUIRE(vertex.has_value());
            REQUIRE(fragment.has_value());

            shader = device.CreateShader({ *vertex, *fragment });
            REQUIRE(shader.IsValid());
            program = device.GetShader(shader)->program;
        }

        ~LitShaderFixture() { device.DestroyShader(shader); }

        LitShaderFixture(const LitShaderFixture&) = delete;
        LitShaderFixture& operator=(const LitShaderFixture&) = delete;
    };
}

GL_TEST_CASE("The lit shader compiles and links")
{
    LitShaderFixture lit;
    CHECK(glIsProgram(lit.program) == GL_TRUE);
}

GL_TEST_CASE("Each uniform block is on the binding UniformBinding names")
{
    LitShaderFixture lit;

    CHECK(Block(lit.program, "Frame").binding == UniformBinding::Frame);
    CHECK(Block(lit.program, "Object").binding == UniformBinding::Object);
    CHECK(Block(lit.program, "Light").binding == UniformBinding::Light);
    CHECK(Block(lit.program, "Material").binding == UniformBinding::Material);
}

GL_TEST_CASE("Each uniform block has the size of its C++ struct")
{
    LitShaderFixture lit;

    CHECK(Block(lit.program, "Frame").size == SizeOf<FrameUniforms>());
    CHECK(Block(lit.program, "Object").size == SizeOf<ObjectUniforms>());
    CHECK(Block(lit.program, "Light").size == SizeOf<LightUniforms>());
    CHECK(Block(lit.program, "Material").size == SizeOf<MaterialUniforms>());
}

GL_TEST_CASE("Each uniform sits at the offset of its C++ member")
{
    LitShaderFixture lit;
    const GLuint p = lit.program;

#define GEF_CHECK_OFFSET(glslName, Struct, member)                             \
    CHECK(Offset(p, glslName) == static_cast<GLint>(offsetof(Struct, member)))

    GEF_CHECK_OFFSET("u_ViewProjection", FrameUniforms, viewProjection);
    GEF_CHECK_OFFSET("u_CameraPosition", FrameUniforms, cameraPosition);

    GEF_CHECK_OFFSET("u_Model", ObjectUniforms, model);
    GEF_CHECK_OFFSET("u_NormalMatrix", ObjectUniforms, normalMatrix);

    GEF_CHECK_OFFSET("u_Direction", LightUniforms, direction);
    GEF_CHECK_OFFSET("u_Color", LightUniforms, color);
    GEF_CHECK_OFFSET("u_Ambient", LightUniforms, ambient);

    GEF_CHECK_OFFSET("u_Albedo", MaterialUniforms, albedo);
    GEF_CHECK_OFFSET("u_Params", MaterialUniforms, params);

#undef GEF_CHECK_OFFSET
}
