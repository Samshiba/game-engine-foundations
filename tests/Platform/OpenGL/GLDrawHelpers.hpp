//
// Path: tests/Platform/OpenGL/GLDrawHelpers.hpp
//
// The smallest pieces a draw needs, for tests that check what Submit does to
// the GL state. A recorded CommandList only reaches GL through draws: a Bind*
// on its own records nothing, so these tests record a draw of zero indices.
// Submit then applies the packet's state without rasterizing anything.
//

#pragma once

#include <string_view>

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/Mesh.hpp>

namespace GEF::Tests
{
    inline constexpr std::string_view MinimalVertexSource = R"(
        #version 450 core
        layout (location = 0) in vec3 a_Position;
        void main() { gl_Position = vec4(a_Position, 1.0); })";

    inline constexpr std::string_view MinimalFragmentSource = R"(
        #version 450 core
        out vec4 o_Color;
        void main() { o_Color = vec4(1.0); })";

    inline Renderer::Mesh UploadTriangle(Renderer::GraphicsDevice& device)
    {
        Renderer::MeshData data;
        data.vertices = { { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0 } },
                          { { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0 } },
                          { { 0, 1, 0 }, { 0, 0, 1 }, { 0, 1 } } };
        data.indices = { 0, 1, 2 };
        return Renderer::UploadMesh(device, data);
    }

    // A device with a minimal shader, a pipeline and a triangle, released at
    // the end of the test
    struct DrawFixture
    {
        Platform::OpenGLGraphicsDevice device;
        Renderer::ShaderHandle shader =
            device.CreateShader({ MinimalVertexSource, MinimalFragmentSource });
        Renderer::PipelineHandle pipeline =
            device.CreatePipeline({ .shader = shader,
                                    .depth = {},
                                    .cull = Renderer::CullMode::None });
        Renderer::Mesh triangle = UploadTriangle(device);

        DrawFixture() = default;
        ~DrawFixture()
        {
            DestroyMesh(device, triangle);
            device.DestroyPipeline(pipeline);
            device.DestroyShader(shader);
        }

        DrawFixture(const DrawFixture&) = delete;
        DrawFixture& operator=(const DrawFixture&) = delete;

        // Records a zero-index draw of the triangle with `with`
        void RecordDraw(Renderer::CommandList& cmd,
                        Renderer::PipelineHandle with) const
        {
            cmd.BindPipeline(with);
            cmd.BindVertexArray(triangle.vertexArray);
            cmd.DrawIndexed(0);
        }

        void RecordDraw(Renderer::CommandList& cmd) const
        {
            RecordDraw(cmd, pipeline);
        }
    };
} // namespace GEF::Tests
