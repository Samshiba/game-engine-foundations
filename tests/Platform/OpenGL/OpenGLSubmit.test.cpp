//
// Path: tests/Platform/OpenGL/OpenGLSubmit.test.cpp
//
// Submit on the OpenGL backend: frame commands first, then the packets in
// sorted order, through a state cache that skips the binds that would not
// change anything. The SubmitStats count what really reached GL.
//

#include <cstdint>

#include <doctest.h>

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/Layout/LitShaderLayout.hpp>

#include "GLDrawHelpers.hpp"
#include "GLTestContext.hpp"

using namespace GEF;
namespace UniformBinding = Renderer::Layout::UniformBinding;
namespace ShaderStorageBinding = Renderer::Layout::ShaderStorageBinding;

namespace
{
    Renderer::BufferHandle CreateUniformBuffer(Renderer::GraphicsDevice& device)
    {
        return device.CreateBuffer({ .type = Renderer::BufferType::Uniform,
                                     .usage = Renderer::BufferUsage::Dynamic,
                                     .size = 64 });
    }

    Renderer::BufferHandle CreateStorageBuffer(Renderer::GraphicsDevice& device)
    {
        return device.CreateBuffer({ .type = Renderer::BufferType::ShaderStorage,
                                     .usage = Renderer::BufferUsage::Dynamic,
                                     .size = 64 });
    }

    GLint CurrentProgram()
    {
        GLint program = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        return program;
    }
} // namespace

GL_TEST_CASE("Draws sharing all their state bind it once")
{
    Tests::DrawFixture fixture;
    const auto frame = CreateUniformBuffer(fixture.device);
    const auto environment = CreateUniformBuffer(fixture.device);
    const auto points = CreateStorageBuffer(fixture.device);
    const auto spots = CreateStorageBuffer(fixture.device);

    // What the SceneRenderer does with instancing off: one draw per entity,
    // same pipeline, same mesh, same per-frame buffers
    Renderer::CommandList cmd;
    cmd.BindUniformBuffer(UniformBinding::Frame, frame);
    cmd.BindUniformBuffer(UniformBinding::Environment, environment);
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::PointLight, points);
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::SpotLight, spots);
    for (int i = 0; i < 100; ++i)
        fixture.RecordDraw(cmd);
    fixture.device.Submit(cmd);

    const auto& stats = fixture.device.GetLastSubmitStats();
    CHECK(stats.drawCalls == 100);
    CHECK(stats.pipelineChanges == 1);
    CHECK(stats.vertexArrayChanges == 1);
    CHECK(stats.uniformBufferChanges == 2); // one per binding, not per draw
    CHECK(stats.shaderStorageBufferChanges == 2);

    fixture.device.DestroyBuffer(spots);
    fixture.device.DestroyBuffer(points);
    fixture.device.DestroyBuffer(environment);
    fixture.device.DestroyBuffer(frame);
}

GL_TEST_CASE("Sorting turns interleaved pipelines into one change each")
{
    Tests::DrawFixture fixture;
    const auto other =
        fixture.device.CreatePipeline({ .shader = fixture.shader,
                                        .depth = {},
                                        .cull = Renderer::CullMode::None });

    // A, B, A, B as recorded: 4 pipeline changes without the sort
    Renderer::CommandList cmd;
    for (int i = 0; i < 2; ++i)
    {
        fixture.RecordDraw(cmd, fixture.pipeline);
        fixture.RecordDraw(cmd, other);
    }
    fixture.device.Submit(cmd);

    CHECK(fixture.device.GetLastSubmitStats().drawCalls == 4);
    CHECK(fixture.device.GetLastSubmitStats().pipelineChanges == 2);

    fixture.device.DestroyPipeline(other);
}

GL_TEST_CASE("The state cache starts empty at every Submit")
{
    // ImGui renders between two Submits and changes the GL state behind the
    // device's back: a cache kept across frames would skip a needed bind
    Tests::DrawFixture fixture;
    Renderer::CommandList cmd;
    fixture.RecordDraw(cmd);
    fixture.device.Submit(cmd);
    const GLint program = CurrentProgram();
    REQUIRE(program != 0);

    glUseProgram(0); // what another renderer could do

    fixture.device.Submit(cmd);
    CHECK(CurrentProgram() == program);
    CHECK(fixture.device.GetLastSubmitStats().pipelineChanges == 1);
}

GL_TEST_CASE("Unused uniform buffer slots are not bound")
{
    Tests::DrawFixture fixture;
    const auto frame = CreateUniformBuffer(fixture.device);

    Renderer::CommandList cmd;
    cmd.BindUniformBuffer(UniformBinding::Frame, frame);
    fixture.RecordDraw(cmd); // nothing on the Environment binding
    fixture.device.Submit(cmd);

    CHECK(fixture.device.GetLastSubmitStats().uniformBufferChanges == 1);

    fixture.device.DestroyBuffer(frame);
}

GL_TEST_CASE("Unused shader storage slots are not bound")
{
    Tests::DrawFixture fixture;
    const auto points = CreateStorageBuffer(fixture.device);

    Renderer::CommandList cmd;
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::PointLight, points);
    fixture.RecordDraw(cmd); // nothing on the directional and spot bindings
    fixture.device.Submit(cmd);

    CHECK(fixture.device.GetLastSubmitStats().shaderStorageBufferChanges == 1);

    fixture.device.DestroyBuffer(points);
}

GL_TEST_CASE("A shader storage buffer changing between draws is rebound")
{
    // What happens when two draws see different light lists: each change
    // costs one bind, the unchanged slots none
    Tests::DrawFixture fixture;
    const auto first = CreateStorageBuffer(fixture.device);
    const auto second = CreateStorageBuffer(fixture.device);

    Renderer::CommandList cmd;
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::PointLight, first);
    fixture.RecordDraw(cmd);
    cmd.BindShaderStorageBuffer(ShaderStorageBinding::PointLight, second);
    fixture.RecordDraw(cmd);
    fixture.device.Submit(cmd);

    CHECK(fixture.device.GetLastSubmitStats().shaderStorageBufferChanges == 2);

    fixture.device.DestroyBuffer(second);
    fixture.device.DestroyBuffer(first);
}

GL_TEST_CASE("Stats describe the last Submit only")
{
    Tests::DrawFixture fixture;
    Renderer::CommandList cmd;
    fixture.RecordDraw(cmd);
    fixture.RecordDraw(cmd);
    fixture.device.Submit(cmd);
    REQUIRE(fixture.device.GetLastSubmitStats().drawCalls == 2);

    cmd.Reset();
    fixture.device.Submit(cmd);

    const auto& stats = fixture.device.GetLastSubmitStats();
    CHECK(stats.drawCalls == 0);
    CHECK(stats.pipelineChanges == 0);
    CHECK(stats.vertexArrayChanges == 0);
    CHECK(stats.uniformBufferChanges == 0);
    CHECK(stats.shaderStorageBufferChanges == 0);
}

GL_TEST_CASE("Frame commands set the viewport and the clear color")
{
    Tests::DrawFixture fixture;
    GLint previous[4] = {};
    glGetIntegerv(GL_VIEWPORT, previous);

    Renderer::CommandList cmd;
    cmd.SetViewport(10, 20, 300, 200);
    cmd.SetClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    fixture.device.Submit(cmd);

    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    CHECK(viewport[0] == 10);
    CHECK(viewport[1] == 20);
    CHECK(viewport[2] == 300);
    CHECK(viewport[3] == 200);

    GLfloat color[4] = {};
    glGetFloatv(GL_COLOR_CLEAR_VALUE, color);
    CHECK(color[0] == doctest::Approx(0.25f));
    CHECK(color[1] == doctest::Approx(0.5f));
    CHECK(color[2] == doctest::Approx(0.75f));

    // The GL context is shared by every test: leave the viewport as it was
    glViewport(previous[0], previous[1], previous[2], previous[3]);
}


GL_TEST_CASE("Submit times the replay on the CPU and on the GPU")
{
    Tests::DrawFixture fixture;
    Renderer::CommandList cmd;

    // The GPU time comes from timestamp queries read two frames later: the
    // first frames have none yet, and must report 0 rather than garbage
    for (int frame = 0; frame < 6; ++frame)
    {
        cmd.Reset();
        for (int i = 0; i < 100; ++i)
            fixture.RecordDraw(cmd);
        fixture.device.Submit(cmd);
        glFinish(); // what the swap does in the engine: the queries complete

        const auto& stats = fixture.device.GetLastSubmitStats();
        CAPTURE(frame);
        CHECK(stats.cpuSubmitTimeMs > 0.0);
        CHECK(stats.cpuSubmitTimeMs < 1000.0);
        CHECK(stats.gpuFrameTimeMs >= 0.0);
        CHECK(stats.gpuFrameTimeMs < 1000.0);
        if (frame < 2)
            CHECK(stats.gpuFrameTimeMs == 0.0);
    }

    CHECK(fixture.device.GetLastSubmitStats().gpuFrameTimeMs > 0.0);
}
