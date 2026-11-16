//
// Path: tests/Platform/OpenGL/OpenGLPipeline.test.cpp
//

#include <doctest.h>

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/Mesh.hpp>

#include "GLDrawHelpers.hpp"
#include "GLTestContext.hpp"

using namespace GEF;
using Renderer::CullMode;

namespace
{

    // The GL state a pipeline is supposed to set
    struct GLState
    {
        GLint program = 0;
        bool depthTest = false;
        bool depthWrite = false;
        bool cullFace = false;
        GLint cullFaceMode = 0;
    };

    GLState ReadGLState()
    {
        GLState state;
        glGetIntegerv(GL_CURRENT_PROGRAM, &state.program);
        state.depthTest = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
        GLboolean depthWrite = GL_FALSE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
        state.depthWrite = depthWrite == GL_TRUE;
        state.cullFace = glIsEnabled(GL_CULL_FACE) == GL_TRUE;
        glGetIntegerv(GL_CULL_FACE_MODE, &state.cullFaceMode);
        return state;
    }

    // A device with one shader, released at the end of the test
    struct PipelineFixture
    {
        Platform::OpenGLGraphicsDevice device;
        Renderer::ShaderHandle shader = device.CreateShader(
            { Tests::MinimalVertexSource, Tests::MinimalFragmentSource });
        Renderer::Mesh triangle = Tests::UploadTriangle(device);

        PipelineFixture()
        {
            REQUIRE(shader.IsValid());
        }
        ~PipelineFixture()
        {
            DestroyMesh(device, triangle);
            device.DestroyShader(shader);
        }

        PipelineFixture(const PipelineFixture&) = delete;
        PipelineFixture& operator=(const PipelineFixture&) = delete;

        Renderer::PipelineHandle Create(bool depthTest, bool depthWrite,
                                        CullMode cull)
        {
            return device.CreatePipeline(
                { .shader = shader,
                  .depth = { .test = depthTest, .write = depthWrite },
                  .cull = cull });
        }

        // A pipeline reaches GL with a draw: a zero-index one applies its
        // state without drawing anything
        void Bind(Renderer::PipelineHandle pipeline)
        {
            Renderer::CommandList cmd;
            cmd.BindPipeline(pipeline);
            cmd.BindVertexArray(triangle.vertexArray);
            cmd.DrawIndexed(0);
            device.Submit(cmd);
        }

        GLint Program() const
        {
            return static_cast<GLint>(device.GetShader(shader)->program);
        }
    };
} // namespace

GL_TEST_CASE("CreatePipeline returns a valid handle")
{
    PipelineFixture fixture;

    const Renderer::PipelineHandle pipeline =
        fixture.Create(true, true, CullMode::Back);

    CHECK(pipeline.IsValid());
    CHECK(fixture.device.GetPipeline(pipeline) != nullptr);

    fixture.device.DestroyPipeline(pipeline);
}

GL_TEST_CASE("BindPipeline applies the shader, depth and cull state")
{
    PipelineFixture fixture;

    SUBCASE("depth test + write, back-face culling (opaque 3D)")
    {
        const auto pipeline = fixture.Create(true, true, CullMode::Back);
        fixture.Bind(pipeline);

        const GLState state = ReadGLState();
        CHECK(state.program == fixture.Program());
        CHECK(state.depthTest);
        CHECK(state.depthWrite);
        CHECK(state.cullFace);
        CHECK(state.cullFaceMode == GL_BACK);

        fixture.device.DestroyPipeline(pipeline);
    }

    SUBCASE("depth test without write, front-face culling")
    {
        const auto pipeline = fixture.Create(true, false, CullMode::Front);
        fixture.Bind(pipeline);

        const GLState state = ReadGLState();
        CHECK(state.depthTest);
        CHECK_FALSE(state.depthWrite);
        CHECK(state.cullFace);
        CHECK(state.cullFaceMode == GL_FRONT);

        fixture.device.DestroyPipeline(pipeline);
    }

    SUBCASE("no depth, no culling (UI, overlays)")
    {
        const auto pipeline = fixture.Create(false, false, CullMode::None);
        fixture.Bind(pipeline);

        const GLState state = ReadGLState();
        CHECK_FALSE(state.depthTest);
        CHECK_FALSE(state.depthWrite);
        CHECK_FALSE(state.cullFace);

        fixture.device.DestroyPipeline(pipeline);
    }
}

GL_TEST_CASE("Switching pipelines leaves no state from the previous one")
{
    // The bug a pipeline object prevents: setters "leak" state between draws
    PipelineFixture fixture;
    const auto overlay = fixture.Create(false, false, CullMode::None);
    const auto opaque = fixture.Create(true, true, CullMode::Back);

    fixture.Bind(overlay);
    fixture.Bind(opaque);
    {
        const GLState state = ReadGLState();
        CHECK(state.depthTest);
        CHECK(state.depthWrite);
        CHECK(state.cullFace);
        CHECK(state.cullFaceMode == GL_BACK);
    }

    fixture.Bind(overlay);
    {
        const GLState state = ReadGLState();
        CHECK_FALSE(state.depthTest);
        CHECK_FALSE(state.depthWrite);
        CHECK_FALSE(state.cullFace);
    }

    fixture.device.DestroyPipeline(opaque);
    fixture.device.DestroyPipeline(overlay);
}

GL_TEST_CASE(
    "A pass clears the depth buffer even after a pipeline without depth write")
{
    // glClear(GL_DEPTH_BUFFER_BIT) obeys glDepthMask: if the last pipeline of
    // a frame disabled depth writes, a naive clear would leave the old depth
    // values, and the whole next frame would be depth-tested against them.
    // Checked on a real depth buffer (a depth-only render target, as a shadow
    // map would be), not just the GL flag.
    PipelineFixture fixture;
    const auto target = fixture.device.CreateRenderTarget(
        { .sizeMode = Renderer::SizeMode::Fixed,
          .width = 4,
          .height = 4,
          .depthStencil = Renderer::TextureFormat::Depth32F });
    REQUIRE(target.IsValid());
    const GLuint framebuffer = fixture.device.GetRenderTarget(target)->id;

    const auto readDepth = [framebuffer] {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        float depth = -1.0f;
        glReadPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return depth;
    };

    // Fill the depth buffer with "old frame" values
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDepthMask(GL_TRUE);
    glClearDepth(0.25);
    glClear(GL_DEPTH_BUFFER_BIT);
    REQUIRE(readDepth() == doctest::Approx(0.25f));

    // Last pipeline of the frame: no depth write
    const auto noDepthWrite = fixture.Create(true, false, CullMode::None);
    fixture.Bind(noDepthWrite);

    // Next frame's pass must still reset the depth buffer
    glClearDepth(1.0);
    Renderer::CommandList cmd;
    cmd.BeginPass({ .target = target, .clearColor = false });
    cmd.EndPass();
    fixture.device.Submit(cmd);

    CHECK(readDepth() == doctest::Approx(1.0f));

    fixture.device.DestroyRenderTarget(target);
    fixture.device.DestroyPipeline(noDepthWrite);
}

GL_TEST_CASE("DestroyPipeline invalidates the handle")
{
    PipelineFixture fixture;
    const auto pipeline = fixture.Create(true, true, CullMode::Back);

    fixture.device.DestroyPipeline(pipeline);

    CHECK(fixture.device.GetPipeline(pipeline) == nullptr);
}
