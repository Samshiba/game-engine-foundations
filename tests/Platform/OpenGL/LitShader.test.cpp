//
// Path: tests/Platform/OpenGL/LitShader.test.cpp
//
// The embedded lit shaders (Engine/embed/shaders/lit.*) against the C++
// mirrors in Layout/LitShaderLayout.hpp. A block whose binding, size, member
// offset or array stride drifts from its struct renders garbage without any
// GL error: this catches it before the screen does (e.g. two blocks bound to
// each other's slot). Uniform blocks are std140, the light SSBOs std430.
//

#include <cstddef>

#include <doctest.h>

#include <Engine/Core/Embedded.hpp>
#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/Layout/LitShaderLayout.hpp>

#include "GLTestContext.hpp"

using namespace GEF;
using namespace GEF::Renderer;
using namespace GEF::Renderer::Layout;

namespace
{
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

    // Shader storage blocks and their members go through the program
    // interface query API: there is no glGetUniformBlockIndex equivalent
    GLint StorageBlockBinding(GLuint program, const char* name)
    {
        const GLuint index =
            glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, name);
        if (index == GL_INVALID_INDEX)
            return -1;
        const GLenum property = GL_BUFFER_BINDING;
        GLint binding = -1;
        glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, 1,
                               &property, 1, nullptr, &binding);
        return binding;
    }

    struct BufferVariableInfo
    {
        GLint offset = -1;
        GLint topLevelArrayStride = -1;
    };

    // A member of a storage block, e.g. "pointLights[0].color"
    BufferVariableInfo BufferVariable(GLuint program, const char* name)
    {
        BufferVariableInfo info;
        const GLuint index =
            glGetProgramResourceIndex(program, GL_BUFFER_VARIABLE, name);
        if (index == GL_INVALID_INDEX)
            return info;
        const GLenum properties[] = { GL_OFFSET, GL_TOP_LEVEL_ARRAY_STRIDE };
        GLint values[2] = { -1, -1 };
        glGetProgramResourceiv(program, GL_BUFFER_VARIABLE, index, 2,
                               properties, 2, nullptr, values);
        info.offset = values[0];
        info.topLevelArrayStride = values[1];
        return info;
    }

    template <typename T>
    GLint SizeOf()
    {
        return static_cast<GLint>(sizeof(T));
    }

    // The lit shader, compiled once per test from the embedded sources
    struct LitShaderFixture
    {
        Platform::OpenGLGraphicsDevice device;
        ShaderHandle shader;
        GLuint program = 0;

        LitShaderFixture()
        {
            shader = device.CreateShader(
                { Embedded::LitVertexSource(), Embedded::LitFragmentSource() });
            REQUIRE(shader.IsValid());
            program = device.GetShader(shader)->program;
        }

        ~LitShaderFixture()
        {
            device.DestroyShader(shader);
        }

        LitShaderFixture(const LitShaderFixture&) = delete;
        LitShaderFixture& operator=(const LitShaderFixture&) = delete;
    };
} // namespace

GL_TEST_CASE("The lit shader compiles and links")
{
    LitShaderFixture lit;
    CHECK(glIsProgram(lit.program) == GL_TRUE);
}

GL_TEST_CASE("Each uniform block is on the binding UniformBinding names")
{
    LitShaderFixture lit;

    CHECK(Block(lit.program, "Frame").binding == UniformBinding::Frame);
    CHECK(Block(lit.program, "Environment").binding
          == UniformBinding::Environment);
}

GL_TEST_CASE("Each uniform block has the size of its C++ struct")
{
    LitShaderFixture lit;

    CHECK(Block(lit.program, "Frame").size == SizeOf<FrameUniforms>());
    CHECK(Block(lit.program, "Environment").size
          == SizeOf<EnvironmentUniforms>());
}

GL_TEST_CASE("Each uniform sits at the offset of its C++ member")
{
    LitShaderFixture lit;
    const GLuint p = lit.program;

#define GEF_CHECK_OFFSET(glslName, Struct, member)                                 CHECK(Offset(p, glslName) == static_cast<GLint>(offsetof(Struct, member)))

    GEF_CHECK_OFFSET("u_ViewProjection", FrameUniforms, viewProjection);
    GEF_CHECK_OFFSET("u_CameraPosition", FrameUniforms, cameraPosition);

    GEF_CHECK_OFFSET("u_Ambient", EnvironmentUniforms, ambient);

#undef GEF_CHECK_OFFSET
}

GL_TEST_CASE("Each light storage block is on the binding ShaderStorageBinding "
             "names")
{
    LitShaderFixture lit;

    CHECK(StorageBlockBinding(lit.program, "DirectionalLights")
          == ShaderStorageBinding::DirectionalLight);
    CHECK(StorageBlockBinding(lit.program, "PointLights")
          == ShaderStorageBinding::PointLight);
    CHECK(StorageBlockBinding(lit.program, "SpotLights")
          == ShaderStorageBinding::SpotLight);
}

GL_TEST_CASE("Each light count sits at the start of its header")
{
    LitShaderFixture lit;
    const GLuint p = lit.program;
    const auto countOffset =
        static_cast<GLint>(offsetof(LightListHeader, count));

    CHECK(BufferVariable(p, "dirCount").offset == countOffset);
    CHECK(BufferVariable(p, "pointCount").offset == countOffset);
    CHECK(BufferVariable(p, "spotCount").offset == countOffset);
}

GL_TEST_CASE("Each light array has the stride of its C++ struct")
{
    // The stride is what an array of structs gets wrong: one byte off and
    // light 1 already reads the end of light 0
    LitShaderFixture lit;
    const GLuint p = lit.program;

    CHECK(BufferVariable(p, "dirLights[0].direction").topLevelArrayStride
          == SizeOf<GpuDirectionalLight>());
    CHECK(BufferVariable(p, "pointLights[0].positionRange").topLevelArrayStride
          == SizeOf<GpuPointLight>());
    CHECK(BufferVariable(p, "spotLights[0].positionRange").topLevelArrayStride
          == SizeOf<GpuSpotLight>());
}

GL_TEST_CASE("Each light member sits after the header, at its C++ offset")
{
    LitShaderFixture lit;
    const GLuint p = lit.program;

    // The array starts right after the 16-byte header
#define GEF_CHECK_LIGHT_OFFSET(glslName, Struct, member)                           CHECK(BufferVariable(p, glslName).offset                                             == static_cast<GLint>(sizeof(LightListHeader)                                                        + offsetof(Struct, member)))

    GEF_CHECK_LIGHT_OFFSET("dirLights[0].direction", GpuDirectionalLight,
                           direction);
    GEF_CHECK_LIGHT_OFFSET("dirLights[0].color", GpuDirectionalLight, color);

    GEF_CHECK_LIGHT_OFFSET("pointLights[0].positionRange", GpuPointLight,
                           positionRange);
    GEF_CHECK_LIGHT_OFFSET("pointLights[0].color", GpuPointLight, color);

    GEF_CHECK_LIGHT_OFFSET("spotLights[0].positionRange", GpuSpotLight,
                           positionRange);
    GEF_CHECK_LIGHT_OFFSET("spotLights[0].direction", GpuSpotLight, direction);
    GEF_CHECK_LIGHT_OFFSET("spotLights[0].color", GpuSpotLight, color);
    GEF_CHECK_LIGHT_OFFSET("spotLights[0].coneCos", GpuSpotLight, coneCos);

#undef GEF_CHECK_LIGHT_OFFSET
}

GL_TEST_CASE(
    "Per-vertex and per-instance attributes sit where the layouts put them")
{
    // CreateVertexArray numbers attributes in order: Vertex::Layout() on
    // binding 0 (0, 1, 2, 3), then InstanceData::Layout() on binding 1 (4...).
    // A matrix takes one location per column. The shader's "location = N"
    // must match, or an instance would read another field's data.
    LitShaderFixture lit;
    const auto location = [&](const char* name) {
        return glGetAttribLocation(lit.program, name);
    };

    CHECK(location("a_Position") == 0);
    CHECK(location("a_Normal") == 1);
    CHECK(location("a_Tangent") == 2);
    CHECK(location("a_UV") == 3);
    CHECK(location("a_Model") == 4); // mat4: 4, 5, 6, 7
    CHECK(location("a_NormalMatrix") == 8); // mat3: 8, 9, 10
    CHECK(location("a_Albedo") == 11);
    CHECK(location("a_Params") == 12);
}
