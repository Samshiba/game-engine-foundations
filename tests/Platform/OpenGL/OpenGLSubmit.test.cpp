//
// Path: tests/Platform/OpenGL/OpenGLSubmit.test.cpp
//
// Submit on the OpenGL backend: pass after pass, bind the target, clear it,
// blit into it, then replay that pass's packets in sorted order, through a
// state cache that skips the binds that would not change anything. The
// SubmitStats count what really reached GL.
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

GL_TEST_CASE("Each pass sets the viewport to the size of its target")
{
    Tests::DrawFixture fixture;
    GLint previous[4] = {};
    glGetIntegerv(GL_VIEWPORT, previous);
    fixture.device.SetBackBufferSize(64, 64);
    const auto target = fixture.device.CreateRenderTarget(
        { .sizeMode = Renderer::SizeMode::Fixed,
          .width = 32,
          .height = 16,
          .attachments = { Renderer::TextureFormat::RGBA8 } });
    REQUIRE(target.IsValid());

    GLint viewport[4] = {};
    Renderer::CommandList cmd;
    // The implicit pass alone: the whole back buffer
    fixture.device.Submit(cmd);
    glGetIntegerv(GL_VIEWPORT, viewport);
    CHECK(viewport[2] == 64);
    CHECK(viewport[3] == 64);

    // Then a pass on the target: the target, not the window
    cmd.Reset();
    cmd.BeginPass({ .target = target });
    cmd.EndPass();
    fixture.device.Submit(cmd);
    glGetIntegerv(GL_VIEWPORT, viewport);
    CHECK(viewport[0] == 0);
    CHECK(viewport[1] == 0);
    CHECK(viewport[2] == 32);
    CHECK(viewport[3] == 16);

    fixture.device.DestroyRenderTarget(target);
    // The GL context is shared by every test: leave the viewport as it was
    glViewport(previous[0], previous[1], previous[2], previous[3]);
}

GL_TEST_CASE("Draws spread over several passes are all submitted")
{
    // Submit walks the sorted keys pass by pass: a draw whose key is read
    // with the wrong pass is silently dropped
    Tests::DrawFixture fixture;
    fixture.device.SetBackBufferSize(64, 64);
    const auto target = fixture.device.CreateRenderTarget(
        { .sizeMode = Renderer::SizeMode::Fixed,
          .width = 8,
          .height = 8,
          .attachments = { Renderer::TextureFormat::RGBA8 } });
    REQUIRE(target.IsValid());

    Renderer::CommandList cmd;
    for (int i = 0; i < 3; ++i)
        fixture.RecordDraw(cmd);
    for (int pass = 0; pass < 3; ++pass)
    {
        cmd.BeginPass({ .target = target });
        for (int i = 0; i < 3; ++i)
            fixture.RecordDraw(cmd);
        cmd.EndPass();
    }
    fixture.device.Submit(cmd);

    CHECK(fixture.device.GetLastSubmitStats().drawCalls == 12);

    fixture.device.DestroyRenderTarget(target);
}

GL_TEST_CASE("Submit leaves the back buffer bound for ImGui")
{
    Tests::DrawFixture fixture;
    fixture.device.SetBackBufferSize(64, 64);
    const auto target = fixture.device.CreateRenderTarget(
        { .sizeMode = Renderer::SizeMode::Fixed,
          .width = 8,
          .height = 8,
          .attachments = { Renderer::TextureFormat::RGBA8 } });
    REQUIRE(target.IsValid());

    Renderer::CommandList cmd;
    cmd.BeginPass({ .target = target });
    cmd.EndPass();
    fixture.device.Submit(cmd);

    GLint bound = -1;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &bound);
    CHECK(bound == 0);

    fixture.device.DestroyRenderTarget(target);
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
