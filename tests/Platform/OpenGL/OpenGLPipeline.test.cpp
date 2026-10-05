//
// Path: tests/Platform/OpenGL/OpenGLPipeline.test.cpp
//

#include <doctest.h>

#include "GLTestContext.hpp"

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>

#include <string_view>

using namespace GEF;
using Renderer::CullMode;

namespace
{
    constexpr std::string_view VertexSource = R"(
        #version 450 core
        layout (location = 0) in vec3 a_Position;
        void main() { gl_Position = vec4(a_Position, 1.0); })";

    constexpr std::string_view FragmentSource = R"(
        #version 450 core
        out vec4 o_Color;
        void main() { o_Color = vec4(1.0); })";

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
        Renderer::ShaderHandle shader =
            device.CreateShader({ VertexSource, FragmentSource });

        PipelineFixture() { REQUIRE(shader.IsValid()); }
        ~PipelineFixture() { device.DestroyShader(shader); }

        PipelineFixture(const PipelineFixture&) = delete;
        PipelineFixture& operator=(const PipelineFixture&) = delete;

        Renderer::PipelineHandle Create(bool depthTest, bool depthWrite,
                                        CullMode cull)
        {
            return device.CreatePipeline({ .shader = shader,
                                           .depth = { .test = depthTest,
                                                      .write = depthWrite },
                                           .cull = cull });
        }

        // Records then submits: stays valid once the list records (GEF-38)
        void Bind(Renderer::PipelineHandle pipeline)
        {
            auto cmd = device.BeginCommandList();
            cmd->BindPipeline(pipeline);
            device.SubmitCommandList(cmd);
        }

        GLint Program() const
        {
            return static_cast<GLint>(device.GetShader(shader)->program);
        }
    };
}

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

GL_TEST_CASE("Clear resets the depth buffer even after a pipeline without depth write")
{
    // glClear(GL_DEPTH_BUFFER_BIT) obeys glDepthMask: if the last pipeline of
    // a frame disabled depth writes, a naive Clear would leave the old depth
    // values, and the whole next frame would be depth-tested against them.
    // Checked on a real depth buffer, not just the GL flag.
    PipelineFixture fixture;

    GLuint framebuffer = 0;
    GLuint depthBuffer = 0;
    glCreateFramebuffers(1, &framebuffer);
    glCreateRenderbuffers(1, &depthBuffer);
    glNamedRenderbufferStorage(depthBuffer, GL_DEPTH_COMPONENT24, 4, 4);
    glNamedFramebufferRenderbuffer(framebuffer, GL_DEPTH_ATTACHMENT,
                                   GL_RENDERBUFFER, depthBuffer);
    glNamedFramebufferDrawBuffer(framebuffer, GL_NONE);
    glNamedFramebufferReadBuffer(framebuffer, GL_NONE);
    REQUIRE(glCheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) ==
            GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    const auto readDepth = [] {
        float depth = -1.0f;
        glReadPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        return depth;
    };

    // Fill the depth buffer with "old frame" values
    glDepthMask(GL_TRUE);
    glClearDepth(0.25);
    glClear(GL_DEPTH_BUFFER_BIT);
    REQUIRE(readDepth() == doctest::Approx(0.25f));

    // Last pipeline of the frame: no depth write
    const auto noDepthWrite = fixture.Create(true, false, CullMode::None);
    fixture.Bind(noDepthWrite);

    // Next frame's clear must still reset the depth buffer
    glClearDepth(1.0);
    auto cmd = fixture.device.BeginCommandList();
    cmd->Clear();
    fixture.device.SubmitCommandList(cmd);

    CHECK(readDepth() == doctest::Approx(1.0f));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &depthBuffer);
    glDeleteFramebuffers(1, &framebuffer);
    fixture.device.DestroyPipeline(noDepthWrite);
}

GL_TEST_CASE("DestroyPipeline invalidates the handle")
{
    PipelineFixture fixture;
    const auto pipeline = fixture.Create(true, true, CullMode::Back);

    fixture.device.DestroyPipeline(pipeline);

    CHECK(fixture.device.GetPipeline(pipeline) == nullptr);
}
