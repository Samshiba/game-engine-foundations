//
// Path: tests/Platform/OpenGL/OpenGLShaderStorageBuffer.test.cpp
//
// Shader storage buffers (SSBOs) on the OpenGL backend:
//   - BufferType::ShaderStorage
//   - CommandList::BindShaderStorageBuffer(uint32_t binding, BufferHandle),
//     applied by Submit with the draws that follow it
// The light lists use them as a header (count) followed by an array, written
// with two UpdateBuffer calls.
//

#include <array>
#include <cstdint>

#include <doctest.h>
#include <glm/glm.hpp>

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/Layout/LitShaderLayout.hpp>

#include "GLDrawHelpers.hpp"
#include "GLTestContext.hpp"

using namespace GEF;
namespace Layout = Renderer::Layout;

namespace
{
    Renderer::BufferHandle
    CreateStorageBuffer(Platform::OpenGLGraphicsDevice& device, uint32_t size)
    {
        return device.CreateBuffer(
            { .type = Renderer::BufferType::ShaderStorage,
              .usage = Renderer::BufferUsage::Dynamic,
              .size = size });
    }

    Renderer::BufferHandle
    CreateUniformBuffer(Platform::OpenGLGraphicsDevice& device, uint32_t size)
    {
        return device.CreateBuffer({ .type = Renderer::BufferType::Uniform,
                                     .usage = Renderer::BufferUsage::Dynamic,
                                     .size = size });
    }

    GLuint BoundBuffer(GLenum target, uint32_t binding)
    {
        GLint bound = 0;
        glGetIntegeri_v(target, binding, &bound);
        return static_cast<GLuint>(bound);
    }
} // namespace

GL_TEST_CASE("A shader storage buffer is created with its type and size")
{
    Platform::OpenGLGraphicsDevice device;

    const Renderer::BufferHandle handle = CreateStorageBuffer(device, 256);

    const Platform::GLBuffer* buffer = device.GetBuffer(handle);
    REQUIRE(buffer != nullptr);
    CHECK(buffer->type == Renderer::BufferType::ShaderStorage);
    CHECK(buffer->size == 256);

    device.DestroyBuffer(handle);
}

GL_TEST_CASE("A header and an array written separately land back to back")
{
    // The SceneRenderer's light upload: count first, then the lights right
    // after the 16-byte header, where std430 puts the array
    Platform::OpenGLGraphicsDevice device;
    constexpr uint32_t Count = 3;
    const Renderer::BufferHandle handle =
        CreateStorageBuffer(device,
                            sizeof(Layout::LightListHeader)
                                + Count * sizeof(Layout::GpuPointLight));

    const Layout::LightListHeader header{ .count = Count, .pad = {} };
    std::array<Layout::GpuPointLight, Count> lights{};
    for (uint32_t i = 0; i < Count; ++i)
        lights[i].positionRange = glm::vec4(float(i), 0, 0, 10.0f);

    device.UpdateBuffer(handle, 0, &header, sizeof(header));
    device.UpdateBuffer(handle, sizeof(header), lights.data(), sizeof(lights));

    const GLuint id = device.GetBuffer(handle)->id;
    Layout::LightListHeader gpuHeader{};
    glGetNamedBufferSubData(id, 0, sizeof(gpuHeader), &gpuHeader);
    CHECK(gpuHeader.count == Count);

    std::array<Layout::GpuPointLight, Count> gpuLights{};
    glGetNamedBufferSubData(id, sizeof(header), sizeof(gpuLights),
                            gpuLights.data());
    for (uint32_t i = 0; i < Count; ++i)
    {
        CAPTURE(i);
        CHECK(gpuLights[i].positionRange == lights[i].positionRange);
    }

    device.DestroyBuffer(handle);
}

GL_TEST_CASE("BindShaderStorageBuffer attaches the buffer to the binding point")
{
    Tests::DrawFixture fixture;
    const Renderer::BufferHandle handle =
        CreateStorageBuffer(fixture.device, 64);

    // The binding is part of the draw's packet, applied by Submit
    Renderer::CommandList cmd;
    cmd.BindShaderStorageBuffer(Layout::ShaderStorageBinding::PointLight,
                                handle);
    fixture.RecordDraw(cmd);
    fixture.device.Submit(cmd);

    CHECK(BoundBuffer(GL_SHADER_STORAGE_BUFFER_BINDING,
                      Layout::ShaderStorageBinding::PointLight)
          == fixture.device.GetBuffer(handle)->id);

    fixture.device.DestroyBuffer(handle);
}

GL_TEST_CASE("Uniform binding N and storage binding N do not collide")
{
    // Two separate sets of binding points in GL: the Frame UBO and the
    // directional light SSBO can both use index 0
    Tests::DrawFixture fixture;
    const auto uniform = CreateUniformBuffer(fixture.device, 64);
    const auto storage = CreateStorageBuffer(fixture.device, 64);

    Renderer::CommandList cmd;
    cmd.BindUniformBuffer(0, uniform);
    cmd.BindShaderStorageBuffer(0, storage);
    fixture.RecordDraw(cmd);
    fixture.device.Submit(cmd);

    CHECK(BoundBuffer(GL_UNIFORM_BUFFER_BINDING, 0)
          == fixture.device.GetBuffer(uniform)->id);
    CHECK(BoundBuffer(GL_SHADER_STORAGE_BUFFER_BINDING, 0)
          == fixture.device.GetBuffer(storage)->id);

    fixture.device.DestroyBuffer(storage);
    fixture.device.DestroyBuffer(uniform);
}
