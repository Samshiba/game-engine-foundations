//
// Path: tests/Platform/OpenGL/OpenGLReadFramebuffer.test.cpp
//
// ReadFramebuffer reads the default framebuffer back as tightly packed RGBA8,
// rows bottom to top as GL stores them: FrameCapture flips them when it writes
// the PNG.
//

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <doctest.h>

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>

#include "GLTestContext.hpp"

using namespace GEF;

namespace
{
    // GLTestContext's hidden window
    constexpr uint32_t Width = 64;
    constexpr uint32_t Height = 64;

    std::array<uint8_t, 4> PixelAt(const std::vector<uint8_t>& pixels,
                                   uint32_t x, uint32_t y)
    {
        const std::size_t offset = (std::size_t(y) * Width + x) * 4;
        return { pixels[offset], pixels[offset + 1], pixels[offset + 2],
                 pixels[offset + 3] };
    }
} // namespace

GL_TEST_CASE("ReadFramebuffer returns 4 bytes per pixel")
{
    Platform::OpenGLGraphicsDevice device;

    CHECK(device.ReadFramebuffer(Width, Height).size() == Width * Height * 4);
}

GL_TEST_CASE("ReadFramebuffer reads what was cleared, alpha included")
{
    Platform::OpenGLGraphicsDevice device;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const auto pixels = device.ReadFramebuffer(Width, Height);

    using Pixel = std::array<uint8_t, 4>;
    CHECK(PixelAt(pixels, 0, 0) == Pixel{ 255, 0, 255, 255 });
    CHECK(PixelAt(pixels, Width - 1, Height - 1) == Pixel{ 255, 0, 255, 255 });
}

GL_TEST_CASE("ReadFramebuffer starts at the bottom row")
{
    // GL's origin is bottom-left: without the flip, screenshots come out
    // upside down
    Platform::OpenGLGraphicsDevice device;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Only the bottom half turns white
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, Width, Height / 2);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST); // the GL context is shared by every test

    const auto pixels = device.ReadFramebuffer(Width, Height);

    CHECK(PixelAt(pixels, 0, 0)[0] == 255); // first row: bottom
    CHECK(PixelAt(pixels, 0, Height - 1)[0] == 0); // last row: top
}
