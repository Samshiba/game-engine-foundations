//
// Path: tests/Platform/OpenGL/OpenGLRenderTarget.test.cpp
//
// Render targets: a framebuffer and the textures attached to it, created by
// the device as one handle. WindowRelative targets follow the back buffer:
// on a resize the device reallocates their textures in place, so the handles
// the game holds stay valid. Passes draw into them, and a blit copies one into
// another, or into the back buffer.
//

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <doctest.h>

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/CommandList.hpp>

#include "GLDrawHelpers.hpp"
#include "GLTestContext.hpp"

using namespace GEF;
using Renderer::RenderTargetDesc;
using Renderer::SizeMode;
using Renderer::TextureFormat;

namespace
{
    // GLTestContext's hidden window
    constexpr uint32_t WindowSize = 64;

    struct Size
    {
        GLint width = 0;
        GLint height = 0;
    };

    Size TextureSize(GLuint texture)
    {
        Size size;
        glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &size.width);
        glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT,
                                     &size.height);
        return size;
    }

    GLint AttachedTexture(GLuint framebuffer, GLenum attachment)
    {
        GLint name = 0;
        glGetNamedFramebufferAttachmentParameteriv(
            framebuffer, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
            &name);
        return name;
    }

    GLint TextureParameter(GLuint texture, GLenum parameter)
    {
        GLint value = 0;
        glGetTextureParameteriv(texture, parameter, &value);
        return value;
    }

    // Reads the first color attachment of a render target back as RGBA8
    std::vector<uint8_t> ReadTarget(const Platform::GLRenderTarget& target)
    {
        std::vector<uint8_t> pixels(std::size_t(target.width) * target.height
                                    * 4);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, target.id);
        glReadPixels(0, 0, GLsizei(target.width), GLsizei(target.height),
                     GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        return pixels;
    }

    std::array<uint8_t, 4> PixelAt(const std::vector<uint8_t>& pixels,
                                   uint32_t width, uint32_t x, uint32_t y)
    {
        const std::size_t offset = (std::size_t(y) * width + x) * 4;
        return { pixels[offset], pixels[offset + 1], pixels[offset + 2],
                 pixels[offset + 3] };
    }

    // A real draw of the fixture's triangle (DrawFixture records zero
    // indices). It covers the bottom-left half of the top-right quadrant.
    void RecordTriangle(const Tests::DrawFixture& fixture,
                        Renderer::CommandList& cmd)
    {
        cmd.BindPipeline(fixture.pipeline);
        cmd.BindVertexArray(fixture.triangle.vertexArray);
        cmd.DrawIndexed(fixture.triangle.indexCount);
    }

    using Pixel = std::array<uint8_t, 4>;
} // namespace

// ---------- Creation ----------

GL_TEST_CASE("A render target is a complete framebuffer with its attachments")
{
    Platform::OpenGLGraphicsDevice device;
    const auto handle = device.CreateRenderTarget(
        { .sizeMode = SizeMode::Fixed,
          .width = 32,
          .height = 16,
          .attachments = { TextureFormat::RGBA8 },
          .depthStencil = TextureFormat::Depth32F });
    REQUIRE(handle.IsValid());

    const auto* target = device.GetRenderTarget(handle);
    REQUIRE(target != nullptr);
    CHECK(target->width == 32);
    CHECK(target->height == 16);
    CHECK(glCheckNamedFramebufferStatus(target->id, GL_FRAMEBUFFER)
          == GL_FRAMEBUFFER_COMPLETE);

    // Each attachment is a texture of the device, at the target's size
    const auto* color = device.GetTexture(target->attachments[0]);
    const auto* depth = device.GetTexture(target->depthStencil);
    REQUIRE(color != nullptr);
    REQUIRE(depth != nullptr);
    CHECK(AttachedTexture(target->id, GL_COLOR_ATTACHMENT0) == GLint(color->id));
    CHECK(AttachedTexture(target->id, GL_DEPTH_ATTACHMENT) == GLint(depth->id));
    CHECK(TextureSize(color->id).width == 32);
    CHECK(TextureSize(color->id).height == 16);
    CHECK(TextureSize(depth->id).width == 32);

    device.DestroyRenderTarget(handle);
}

GL_TEST_CASE("Color attachments keep their slot, holes included")
{
    // attachments[i] goes to GL_COLOR_ATTACHMENTi, and fragment output i
    // writes to it: a hole stays a hole instead of shifting the others down
    Platform::OpenGLGraphicsDevice device;
    const auto handle = device.CreateRenderTarget(
        { .sizeMode = SizeMode::Fixed,
          .width = 8,
          .height = 8,
          .attachments = { TextureFormat::None, TextureFormat::RGBA8 } });
    REQUIRE(handle.IsValid());
    const auto* target = device.GetRenderTarget(handle);
    REQUIRE(target != nullptr);

    CHECK_FALSE(target->attachments[0].IsValid());
    REQUIRE(target->attachments[1].IsValid());
    CHECK(AttachedTexture(target->id, GL_COLOR_ATTACHMENT0) == 0);
    CHECK(AttachedTexture(target->id, GL_COLOR_ATTACHMENT1)
          == GLint(device.GetTexture(target->attachments[1])->id));

    glBindFramebuffer(GL_FRAMEBUFFER, target->id);
    GLint drawBuffer0 = -1;
    GLint drawBuffer1 = -1;
    glGetIntegerv(GL_DRAW_BUFFER0, &drawBuffer0);
    glGetIntegerv(GL_DRAW_BUFFER1, &drawBuffer1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CHECK(drawBuffer0 == GL_NONE);
    CHECK(drawBuffer1 == GL_COLOR_ATTACHMENT1);

    device.DestroyRenderTarget(handle);
}

GL_TEST_CASE("A depth-only render target is complete")
{
    // What a shadow map needs: no color at all
    Platform::OpenGLGraphicsDevice device;
    const auto handle = device.CreateRenderTarget(
        { .sizeMode = SizeMode::Fixed,
          .width = 16,
          .height = 16,
          .depthStencil = TextureFormat::Depth32F });
    REQUIRE(handle.IsValid());
    const auto* target = device.GetRenderTarget(handle);
    REQUIRE(target != nullptr);

    CHECK(glCheckNamedFramebufferStatus(target->id, GL_FRAMEBUFFER)
          == GL_FRAMEBUFFER_COMPLETE);
    CHECK(target->depthStencil.IsValid());

    device.DestroyRenderTarget(handle);
}

GL_TEST_CASE("Attachments have one mip level and clamp to their edges")
{
    // A render target is written whole every frame: mips would never be
    // filled, and REPEAT would bleed the opposite edge in when it is sampled
    Platform::OpenGLGraphicsDevice device;
    const auto handle = device.CreateRenderTarget(
        { .sizeMode = SizeMode::Fixed,
          .width = 256,
          .height = 256,
          .attachments = { TextureFormat::RGBA8 },
          .depthStencil = TextureFormat::Depth32F });
    REQUIRE(handle.IsValid());
    const auto* target = device.GetRenderTarget(handle);
    REQUIRE(target != nullptr);

    for (const auto texture : { target->attachments[0], target->depthStencil })
    {
        const GLuint id = device.GetTexture(texture)->id;
        CHECK(TextureParameter(id, GL_TEXTURE_IMMUTABLE_LEVELS) == 1);
        CHECK(TextureParameter(id, GL_TEXTURE_WRAP_S) == GL_CLAMP_TO_EDGE);
        CHECK(TextureParameter(id, GL_TEXTURE_WRAP_T) == GL_CLAMP_TO_EDGE);
    }

    device.DestroyRenderTarget(handle);
}

GL_TEST_CASE("DestroyRenderTarget releases the target and its textures")
{
    Platform::OpenGLGraphicsDevice device;
    const auto handle = device.CreateRenderTarget(
        { .sizeMode = SizeMode::Fixed,
          .width = 8,
          .height = 8,
          .attachments = { TextureFormat::RGBA8 },
          .depthStencil = TextureFormat::Depth32F });
    REQUIRE(handle.IsValid());
    const auto color = device.GetRenderTarget(handle)->attachments[0];
    const auto depth = device.GetRenderTarget(handle)->depthStencil;

    device.DestroyRenderTarget(handle);

    CHECK(device.GetRenderTarget(handle) == nullptr);
    CHECK(device.GetTexture(color) == nullptr);
    CHECK(device.GetTexture(depth) == nullptr);
}

// ---------- Back buffer size ----------

GL_TEST_CASE("A window-relative target is sized from the back buffer")
{
    Platform::OpenGLGraphicsDevice device;
    device.SetBackBufferSize(64, 48);

    const auto half = device.CreateRenderTarget(
        { .sizeMode = SizeMode::WindowRelative,
          .scaleX = 0.5f,
          .scaleY = 0.5f,
          .attachments = { TextureFormat::RGBA8 } });
    REQUIRE(half.IsValid());

    CHECK(device.GetRenderTarget(half)->width == 32);
    CHECK(device.GetRenderTarget(half)->height == 24);

    device.DestroyRenderTarget(half);
}

GL_TEST_CASE("A resize reallocates window-relative targets in place")
{
    Platform::OpenGLGraphicsDevice device;
    device.SetBackBufferSize(64, 64);
    const auto handle = device.CreateRenderTarget(
        { .sizeMode = SizeMode::WindowRelative,
          .attachments = { TextureFormat::RGBA8 },
          .depthStencil = TextureFormat::Depth32F });
    REQUIRE(handle.IsValid());
    const auto color = device.GetRenderTarget(handle)->attachments[0];
    const auto depth = device.GetRenderTarget(handle)->depthStencil;

    device.SetBackBufferSize(50, 30);

    // Same handles: the game never has to recreate them
    const auto* target = device.GetRenderTarget(handle);
    REQUIRE(target != nullptr);
    CHECK(target->attachments[0] == color);
    CHECK(target->depthStencil == depth);

    // New size, everywhere it is stored and in GL
    CHECK(target->width == 50);
    CHECK(target->height == 30);
    const GLuint colorId = device.GetTexture(color)->id;
    const GLuint depthId = device.GetTexture(depth)->id;
    CHECK(device.GetTexture(color)->width == 50);
    CHECK(TextureSize(colorId).width == 50);
    CHECK(TextureSize(colorId).height == 30);
    CHECK(TextureSize(depthId).width == 50);
    CHECK(TextureSize(depthId).height == 30);

    // Still attached, still complete, still clamped
    CHECK(AttachedTexture(target->id, GL_COLOR_ATTACHMENT0) == GLint(colorId));
    CHECK(AttachedTexture(target->id, GL_DEPTH_ATTACHMENT) == GLint(depthId));
    CHECK(glCheckNamedFramebufferStatus(target->id, GL_FRAMEBUFFER)
          == GL_FRAMEBUFFER_COMPLETE);
    CHECK(TextureParameter(colorId, GL_TEXTURE_WRAP_S) == GL_CLAMP_TO_EDGE);

    device.DestroyRenderTarget(handle);
}

GL_TEST_CASE("A resize leaves fixed-size targets alone")
{
    Platform::OpenGLGraphicsDevice device;
    device.SetBackBufferSize(64, 64);
    const auto handle = device.CreateRenderTarget(
        { .sizeMode = SizeMode::Fixed,
          .width = 16,
          .height = 16,
          .attachments = { TextureFormat::RGBA8 } });
    REQUIRE(handle.IsValid());
    const GLuint colorId =
        device.GetTexture(device.GetRenderTarget(handle)->attachments[0])->id;

    device.SetBackBufferSize(128, 128);

    CHECK(device.GetRenderTarget(handle)->width == 16);
    CHECK(device.GetTexture(device.GetRenderTarget(handle)->attachments[0])->id
          == colorId);

    device.DestroyRenderTarget(handle);
}

GL_TEST_CASE("A minimized window (size 0) keeps the targets as they were")
{
    Platform::OpenGLGraphicsDevice device;
    device.SetBackBufferSize(64, 64);
    const auto handle = device.CreateRenderTarget(
        { .sizeMode = SizeMode::WindowRelative,
          .attachments = { TextureFormat::RGBA8 } });
    REQUIRE(handle.IsValid());

    device.SetBackBufferSize(0, 0);

    CHECK(device.GetRenderTarget(handle)->width == 64);
    CHECK(device.GetRenderTarget(handle)->height == 64);

    device.DestroyRenderTarget(handle);
}

// ---------- Passes ----------

GL_TEST_CASE("A pass draws into its target, not into the back buffer")
{
    Tests::DrawFixture fixture;
    fixture.device.SetBackBufferSize(WindowSize, WindowSize);
    const auto handle = fixture.device.CreateRenderTarget(
        { .sizeMode = SizeMode::WindowRelative,
          .attachments = { TextureFormat::RGBA8 } });
    REQUIRE(handle.IsValid());

    Renderer::CommandList cmd;
    cmd.BeginPass({ .target = handle, .clearValue = { 1, 0, 0, 1 } });
    RecordTriangle(fixture, cmd);
    cmd.EndPass();
    fixture.device.Submit(cmd);

    // Top-right quadrant, inside the triangle; bottom-left, outside
    const auto target = ReadTarget(*fixture.device.GetRenderTarget(handle));
    CHECK(PixelAt(target, WindowSize, 40, 36) == Pixel{ 255, 255, 255, 255 });
    CHECK(PixelAt(target, WindowSize, 8, 8) == Pixel{ 255, 0, 0, 255 });

    // The back buffer only got the implicit pass's clear
    const auto screen = fixture.device.ReadFramebuffer(WindowSize, WindowSize);
    CHECK(PixelAt(screen, WindowSize, 40, 36) == Pixel{ 26, 26, 26, 255 });

    fixture.device.DestroyRenderTarget(handle);
}

GL_TEST_CASE("Passes on the same target run in recording order")
{
    // The second pass does not clear: it draws over what the first one left
    Tests::DrawFixture fixture;
    fixture.device.SetBackBufferSize(WindowSize, WindowSize);
    const auto handle = fixture.device.CreateRenderTarget(
        { .sizeMode = SizeMode::WindowRelative,
          .attachments = { TextureFormat::RGBA8 } });
    REQUIRE(handle.IsValid());

    Renderer::CommandList cmd;
    cmd.BeginPass({ .target = handle, .clearValue = { 0, 0, 1, 1 } });
    cmd.EndPass();
    cmd.BeginPass(
        { .target = handle, .clearColor = false, .clearDepth = false });
    RecordTriangle(fixture, cmd);
    cmd.EndPass();
    fixture.device.Submit(cmd);

    const auto target = ReadTarget(*fixture.device.GetRenderTarget(handle));
    CHECK(PixelAt(target, WindowSize, 40, 36) == Pixel{ 255, 255, 255, 255 });
    CHECK(PixelAt(target, WindowSize, 8, 8) == Pixel{ 0, 0, 255, 255 });

    fixture.device.DestroyRenderTarget(handle);
}

GL_TEST_CASE("A blit copies a target into another one of a different size")
{
    Tests::DrawFixture fixture;
    fixture.device.SetBackBufferSize(WindowSize, WindowSize);
    const auto source = fixture.device.CreateRenderTarget(
        { .sizeMode = SizeMode::Fixed,
          .width = 8,
          .height = 8,
          .attachments = { TextureFormat::RGBA8 } });
    const auto destination = fixture.device.CreateRenderTarget(
        { .sizeMode = SizeMode::Fixed,
          .width = 32,
          .height = 32,
          .attachments = { TextureFormat::RGBA8 } });
    REQUIRE(source.IsValid());
    REQUIRE(destination.IsValid());

    Renderer::CommandList cmd;
    cmd.BeginPass({ .target = source, .clearValue = { 0, 1, 0, 1 } });
    cmd.EndPass();
    cmd.BeginPass({ .target = destination, .clearValue = { 1, 0, 0, 1 } });
    cmd.Blit(source); // after the clear: the copy is what is left
    cmd.EndPass();
    fixture.device.Submit(cmd);

    // Stretched over the whole destination, corners included
    const auto pixels = ReadTarget(*fixture.device.GetRenderTarget(destination));
    CHECK(PixelAt(pixels, 32, 0, 0) == Pixel{ 0, 255, 0, 255 });
    CHECK(PixelAt(pixels, 32, 31, 31) == Pixel{ 0, 255, 0, 255 });

    fixture.device.DestroyRenderTarget(destination);
    fixture.device.DestroyRenderTarget(source);
}

GL_TEST_CASE("Rendering into a target then blitting it matches direct rendering")
{
    // GEF-52's acceptance: going through a render target changes nothing on
    // screen, pixel for pixel
    Tests::DrawFixture fixture;
    fixture.device.SetBackBufferSize(WindowSize, WindowSize);
    const auto scene = fixture.device.CreateRenderTarget(
        { .sizeMode = SizeMode::WindowRelative,
          .attachments = { TextureFormat::RGBA8 },
          .depthStencil = TextureFormat::Depth32F });
    REQUIRE(scene.IsValid());

    // Direct: the implicit pass on the back buffer
    Renderer::CommandList cmd;
    RecordTriangle(fixture, cmd);
    fixture.device.Submit(cmd);
    const auto direct = fixture.device.ReadFramebuffer(WindowSize, WindowSize);

    // Through the target, with the same clear color as the implicit pass
    cmd.Reset();
    cmd.BeginPass({ .target = scene, .clearValue = { 0.1f, 0.1f, 0.1f, 1 } });
    RecordTriangle(fixture, cmd);
    cmd.EndPass();
    cmd.BeginPass({ .clearColor = false, .clearDepth = false });
    cmd.Blit(scene);
    cmd.EndPass();
    fixture.device.Submit(cmd);
    const auto blitted = fixture.device.ReadFramebuffer(WindowSize, WindowSize);

    // Something was drawn, and the two frames are identical
    CHECK(PixelAt(direct, WindowSize, 40, 36) == Pixel{ 255, 255, 255, 255 });
    CHECK(blitted == direct);

    fixture.device.DestroyRenderTarget(scene);
}
