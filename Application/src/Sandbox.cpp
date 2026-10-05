//
// Created by genin on 26/01/2026.
// Path: Application/src/Sandbox.cpp
//

#include <Engine/Core/Engine.hpp>
#include <Engine/Core/Log.hpp>
#include <Engine/Core/FileSystem.hpp>

using namespace GEF::Renderer;

struct Sandbox
{
    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
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
            { BufferType::Vertex, sizeof(vertices), vertices });
        indexBuffer = ctx.device.CreateBuffer(
            { BufferType::Index, sizeof(indices), indices });
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
    }

    void OnUpdate(GEF::EngineContext&, float)
    {
    }

    void OnRender(GEF::EngineContext&, CommandList& cmd)
    {
        cmd.BindShader(shader);
        cmd.BindVertexArray(triangle);
        cmd.DrawIndexed(indexCount);
    }

    void OnShutdown(GEF::EngineContext& ctx)
    {
        ctx.device.DestroyVertexArray(triangle);
        ctx.device.DestroyBuffer(indexBuffer);
        ctx.device.DestroyBuffer(vertexBuffer);
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
