//
// Created by genin on 26/01/2026.
// Path: Application/src/Sandbox.cpp
//

#include <Engine/Core/Engine.hpp>
#include <Engine/Core/Log.hpp>

struct Sandbox
{
    std::shared_ptr<GEF::Renderer::VertexArray> triangle;

    void OnInit(GEF::EngineContext& ctx)
    {
        float vertices[] = {
            // position          // color
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        uint32_t indices[] = { 0, 1, 2 };

        auto vertexBuffer = ctx.device.CreateVertexBuffer(vertices,
            sizeof(vertices));
        vertexBuffer->SetLayout({
            { GEF::Renderer::ShaderDataType::Float3, "a_Position" },
            { GEF::Renderer::ShaderDataType::Float3, "a_Color" },
        });

        triangle = ctx.device.CreateVertexArray();
        triangle->AddVertexBuffer(vertexBuffer);
        triangle->SetIndexBuffer(ctx.device.CreateIndexBuffer(indices, 3));
    }

    void OnUpdate(GEF::EngineContext&, float)
    {
    }

    void OnRender(GEF::EngineContext&, GEF::Renderer::CommandList& cmd)
    {
        cmd.BindVertexArray(triangle);
        cmd.DrawIndexed(triangle->GetIndexBuffer()->GetCount());
    }
};

int main()
{
    GEF::Log::Init();

    GEF::Engine engine({ .title = "Sandbox" });
    Sandbox game;
    return engine.Run(game);
}
