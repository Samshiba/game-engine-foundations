//
// Created by genin on 26/01/2026.
// Path: Application/src/Sandbox.cpp
//

#include <Engine/Core/Engine.hpp>
#include <Engine/Core/Log.hpp>
#include <Engine/Core/FileSystem.hpp>
#include <Engine/Event/ApplicationEvent.hpp>
#include <Engine/Renderer/Camera.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iterator>

#include "FreeFlyController.hpp"

using namespace GEF::Renderer;

// Mirrors the std140 block "Frame" in basic.vert
struct FrameData
{
    glm::mat4 viewProjection;
    glm::mat4 model;
};

static_assert(sizeof(FrameData) == 128, "Must match the std140 block size");

namespace
{
    // A unit cube with one color per face. 24 vertices, not 8: a corner is
    // shared by 3 faces of different colors (and later different normals).
    // Every face is counter-clockwise seen from outside, which is what
    // CullMode::Back expects (OpenGL's default front face is CCW).
    // clang-format off
    constexpr float CubeVertices[] = {
        // position             // color
        // +Z (front, red)
        -0.5f, -0.5f,  0.5f,    0.9f, 0.2f, 0.2f,
         0.5f, -0.5f,  0.5f,    0.9f, 0.2f, 0.2f,
         0.5f,  0.5f,  0.5f,    0.9f, 0.2f, 0.2f,
        -0.5f,  0.5f,  0.5f,    0.9f, 0.2f, 0.2f,
        // -Z (back, cyan)
         0.5f, -0.5f, -0.5f,    0.2f, 0.8f, 0.9f,
        -0.5f, -0.5f, -0.5f,    0.2f, 0.8f, 0.9f,
        -0.5f,  0.5f, -0.5f,    0.2f, 0.8f, 0.9f,
         0.5f,  0.5f, -0.5f,    0.2f, 0.8f, 0.9f,
        // +X (right, green)
         0.5f, -0.5f,  0.5f,    0.2f, 0.8f, 0.3f,
         0.5f, -0.5f, -0.5f,    0.2f, 0.8f, 0.3f,
         0.5f,  0.5f, -0.5f,    0.2f, 0.8f, 0.3f,
         0.5f,  0.5f,  0.5f,    0.2f, 0.8f, 0.3f,
        // -X (left, magenta)
        -0.5f, -0.5f, -0.5f,    0.8f, 0.3f, 0.8f,
        -0.5f, -0.5f,  0.5f,    0.8f, 0.3f, 0.8f,
        -0.5f,  0.5f,  0.5f,    0.8f, 0.3f, 0.8f,
        -0.5f,  0.5f, -0.5f,    0.8f, 0.3f, 0.8f,
        // +Y (top, blue)
        -0.5f,  0.5f,  0.5f,    0.3f, 0.4f, 0.9f,
         0.5f,  0.5f,  0.5f,    0.3f, 0.4f, 0.9f,
         0.5f,  0.5f, -0.5f,    0.3f, 0.4f, 0.9f,
        -0.5f,  0.5f, -0.5f,    0.3f, 0.4f, 0.9f,
        // -Y (bottom, yellow)
        -0.5f, -0.5f, -0.5f,    0.9f, 0.8f, 0.2f,
         0.5f, -0.5f, -0.5f,    0.9f, 0.8f, 0.2f,
         0.5f, -0.5f,  0.5f,    0.9f, 0.8f, 0.2f,
        -0.5f, -0.5f,  0.5f,    0.9f, 0.8f, 0.2f,
    };

    // Two triangles per face: (0, 1, 2) and (0, 2, 3), offset by 4 per face
    constexpr uint32_t CubeIndices[] = {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23,
    };
    // clang-format on
}

struct Sandbox
{
    float time = 0.0f;

    Camera camera;
    FreeFlyController freeFlyController;
    GEF::Events::EventBus::SubscriberID resizeSubscription;

    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
    BufferHandle uniformBuffer;
    VertexArrayHandle cube;
    ShaderHandle shader;
    PipelineHandle pipeline;
    uint32_t indexCount = 0;

    void OnInit(GEF::EngineContext& ctx)
    {
        camera.aspectRatio = float(ctx.window.GetWidth()) / float(
            ctx.window.GetHeight());

        resizeSubscription = ctx.events.Subscribe(
            GEF::Events::WindowResizeEvent::GetStaticEventType(),
            [this](GEF::Events::Event const& e) {
                const auto& resize = static_cast<GEF::Events::WindowResizeEvent
                    const&>(e);
                if (resize.GetHeight() == 0)
                    return;
                camera.aspectRatio = float(resize.GetWidth()) / float(
                    resize.GetHeight());
            });

        vertexBuffer = ctx.device.CreateBuffer(
        { .type = BufferType::Vertex,
          .usage = BufferUsage::Static,
          .size = sizeof(CubeVertices),
          .data = CubeVertices });
        indexBuffer = ctx.device.CreateBuffer(
        { .type = BufferType::Index,
          .usage = BufferUsage::Static,
          .size = sizeof(CubeIndices),
          .data = CubeIndices });
        indexCount = static_cast<uint32_t>(std::size(CubeIndices));

        cube = ctx.device.CreateVertexArray({
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

        // Depth test + back-face culling: the faces behind are hidden
        pipeline = ctx.device.CreatePipeline({ .shader = shader,
                                               .depth = { .test = true,
                                                   .write = true },
                                               .cull = CullMode::Back });

        uniformBuffer = ctx.device.CreateBuffer(
        { .type = BufferType::Uniform,
          .usage = BufferUsage::Dynamic,
          .size = sizeof(FrameData) });
    }

    void OnUpdate(GEF::EngineContext& ctx, float dt)
    {
        time += dt;

        freeFlyController.Update(camera, ctx.input, ctx.window, dt);

        const FrameData frameData{ .viewProjection = camera.GetViewProjection(),
                                   // A tilted axis shows 3 faces at once
                                   .model = glm::rotate(
                                       glm::mat4(1.0f), time,
                                       glm::normalize(
                                           glm::vec3(1.0f, 1.0f, 0.0f))) };
        ctx.device.UpdateBuffer(uniformBuffer, 0, &frameData,
                                sizeof(frameData));
    }

    void OnRender(GEF::EngineContext&, CommandList& cmd)
    {
        cmd.BindPipeline(pipeline);
        cmd.BindVertexArray(cube);
        cmd.BindUniformBuffer(0, uniformBuffer);
        cmd.DrawIndexed(indexCount);
    }

    void OnShutdown(GEF::EngineContext& ctx)
    {
        ctx.events.Unsubscribe(
            GEF::Events::WindowResizeEvent::GetStaticEventType(),
            resizeSubscription);

        ctx.device.DestroyPipeline(pipeline);
        ctx.device.DestroyVertexArray(cube);
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
