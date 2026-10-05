//
// Created by genin on 26/01/2026.
// Path: Application/src/Sandbox.cpp
//

#include <Engine/Core/Engine.hpp>
#include <Engine/Core/Log.hpp>
#include <Engine/Core/FileSystem.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace GEF::Renderer;

// Mirrors the std140 block "Frame" in basic.vert
struct FrameData
{
    glm::mat4 transform;
};

static_assert(sizeof(FrameData) == 64, "Must match the std140 block size");

struct Sandbox
{
    float time = 0.0f;

    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
    BufferHandle uniformBuffer;
    VertexArrayHandle triangle;
    ShaderHandle shader;
    uint32_t indexCount = 0;

    void OnInit(GEF::EngineContext& ctx)
    {
        const float vertices[] = {
            // position          // color
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        const uint32_t indices[] = { 0, 1, 2 };

        vertexBuffer = ctx.device.CreateBuffer(
        { BufferType::Vertex, BufferUsage::Static, sizeof(vertices),
          vertices });
        indexBuffer = ctx.device.CreateBuffer(
        { BufferType::Index, BufferUsage::Static, sizeof(indices),
          indices });
        indexCount = 3;

        triangle = ctx.device.CreateVertexArray({
            .vertexBuffers = {
                { .buffer = vertexBuffer,
                  .layout = { { ShaderDataType::Float3, "a_Position" },
                              { ShaderDataType::Float3, "a_Color" } } },
            },
            .indexBuffer = indexBuffer,
        });

        const auto vs = GEF::FileSystem::ReadTextFile(
            GEF::FileSystem::AssetPath("shaders/basic.vert"));
        const auto fs = GEF::FileSystem::ReadTextFile(
            GEF::FileSystem::AssetPath("shaders/basic.frag"));

        shader = ctx.device.CreateShader({ vs.value_or(""),
                                           fs.value_or("") });

        uniformBuffer = ctx.device.CreateBuffer(
        { BufferType::Uniform, BufferUsage::Dynamic, sizeof(FrameData),
          nullptr });
    }

    void OnUpdate(GEF::EngineContext& ctx, float dt)
    {
        time += dt;
        const FrameData frameData{ glm::rotate(glm::mat4(1.0f), time,
                                               glm::vec3(0.0f, 0.0f, 1.0f)) };
        ctx.device.UpdateBuffer(uniformBuffer, 0, &frameData,
                                sizeof(frameData));
    }

    void OnRender(GEF::EngineContext&, CommandList& cmd)
    {
        cmd.BindShader(shader);
        cmd.BindVertexArray(triangle);
        cmd.BindUniformBuffer(0, uniformBuffer);
        cmd.DrawIndexed(indexCount);
    }

    void OnShutdown(GEF::EngineContext& ctx)
    {
        ctx.device.DestroyVertexArray(triangle);
        ctx.device.DestroyBuffer(indexBuffer);
        ctx.device.DestroyBuffer(vertexBuffer);
        ctx.device.DestroyBuffer(uniformBuffer);
        ctx.device.DestroyShader(shader);
    }
};

int main()
{
    GEF::Engine engine({ .title = "Sandbox" });
    Sandbox game;
    const int status = engine.Run(game);

    GEF_INFO("Sandbox finished with status: {}", status);
    return status;
}
