//
// Path: tests/Platform/OpenGL/OpenGLUniformBuffer.test.cpp
//
// Contract for the GEF-32 step 4 API (add this file to tests/CMakeLists.txt
// once it exists):
//   - BufferType::Uniform
//   - GraphicsDevice::UpdateBuffer(BufferHandle, uint32_t offset,
//                                  const void* data, uint32_t size)
//   - CommandList::BindUniformBuffer(uint32_t binding, BufferHandle)
//

#include <doctest.h>

#include "GLTestContext.hpp"

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>

#include <glm/glm.hpp>

#include <array>
#include <cstdint>

using namespace GEF;

namespace
{
    // Same layout as a std140 block { mat4 u_Transform; vec4 u_Tint; }
    struct FrameData
    {
        glm::mat4 transform;
        glm::vec4 tint;
    };

    Renderer::BufferHandle CreateUniformBuffer(
        Platform::OpenGLGraphicsDevice& device, uint32_t size)
    {
        return device.CreateBuffer(
            { Renderer::BufferType::Uniform, size, nullptr });
    }

    template <typename T>
    T ReadBack(const Platform::OpenGLGraphicsDevice& device,
               Renderer::BufferHandle handle, uint32_t offset = 0)
    {
        T value{};
        glGetNamedBufferSubData(device.GetBuffer(handle)->id, offset,
                                sizeof(T), &value);
        return value;
    }
}

GL_TEST_CASE("A uniform buffer is created with its type and size")
{
    Platform::OpenGLGraphicsDevice device;

    const Renderer::BufferHandle handle =
        CreateUniformBuffer(device, sizeof(FrameData));

    const Platform::GLBuffer* buffer = device.GetBuffer(handle);
    REQUIRE(buffer != nullptr);
    CHECK(buffer->type == Renderer::BufferType::Uniform);
    CHECK(buffer->size == sizeof(FrameData));

    device.DestroyBuffer(handle);
}

GL_TEST_CASE("UpdateBuffer replaces the whole content")
{
    Platform::OpenGLGraphicsDevice device;
    const Renderer::BufferHandle handle =
        CreateUniformBuffer(device, sizeof(FrameData));

    const FrameData frame{ glm::mat4(2.0f), glm::vec4(1.0f, 0.5f, 0.25f, 1.0f) };
    device.UpdateBuffer(handle, 0, &frame, sizeof(frame));

    const auto gpu = ReadBack<FrameData>(device, handle);
    CHECK(gpu.transform == frame.transform);
    CHECK(gpu.tint == frame.tint);

    device.DestroyBuffer(handle);
}

GL_TEST_CASE("UpdateBuffer with an offset only touches that range")
{
    Platform::OpenGLGraphicsDevice device;
    const Renderer::BufferHandle handle =
        CreateUniformBuffer(device, sizeof(FrameData));

    const FrameData initial{ glm::mat4(1.0f), glm::vec4(0.0f) };
    device.UpdateBuffer(handle, 0, &initial, sizeof(initial));

    // Only the tint, which starts right after the 64-byte matrix
    const glm::vec4 tint(0.1f, 0.2f, 0.3f, 0.4f);
    device.UpdateBuffer(handle, offsetof(FrameData, tint), &tint, sizeof(tint));

    const auto gpu = ReadBack<FrameData>(device, handle);
    CHECK(gpu.transform == initial.transform); // untouched
    CHECK(gpu.tint == tint);

    device.DestroyBuffer(handle);
}

GL_TEST_CASE("UpdateBuffer can be called every frame")
{
    Platform::OpenGLGraphicsDevice device;
    const Renderer::BufferHandle handle =
        CreateUniformBuffer(device, sizeof(glm::vec4));

    for (int frame = 0; frame < 3; ++frame)
    {
        const glm::vec4 value(static_cast<float>(frame));
        device.UpdateBuffer(handle, 0, &value, sizeof(value));
        CAPTURE(frame);
        CHECK(ReadBack<glm::vec4>(device, handle) == value);
    }

    device.DestroyBuffer(handle);
}

GL_TEST_CASE("BindUniformBuffer attaches the buffer to the binding point")
{
    Platform::OpenGLGraphicsDevice device;
    const Renderer::BufferHandle handle =
        CreateUniformBuffer(device, sizeof(FrameData));

    auto cmd = device.BeginCommandList();
    cmd->BindUniformBuffer(3, handle);
    device.SubmitCommandList(cmd); // still valid once the list records (GEF-38)

    // binding = 3 in GLSL reads whatever is attached to slot 3
    GLint bound = 0;
    glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 3, &bound);
    CHECK(static_cast<GLuint>(bound) == device.GetBuffer(handle)->id);

    device.DestroyBuffer(handle);
}
