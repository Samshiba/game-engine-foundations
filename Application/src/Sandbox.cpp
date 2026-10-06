//
// Created by genin on 26/01/2026.
// Path: Application/src/Sandbox.cpp
//

#include <Engine/Core/Engine.hpp>
#include <Engine/Core/Log.hpp>
#include <Engine/Core/FileSystem.hpp>
#include <Engine/Event/ApplicationEvent.hpp>
#include <Engine/Renderer/Camera.hpp>
#include <Engine/Assets/ObjLoader.hpp>
#include <Engine/Renderer/Mesh.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <optional>
#include <string>
#include <string_view>

#include "FreeFlyController.hpp"

using namespace GEF::Renderer;

// Mirrors the std140 block "Frame" in mesh.vert
struct FrameData
{
    glm::mat4 viewProjection;
    glm::mat4 model;
};

static_assert(sizeof(FrameData) == 128, "Must match the std140 block size");

namespace
{
    std::optional<ShaderHandle> LoadShader(GraphicsDevice& device,
                                           std::string_view name)
    {
        using GEF::FileSystem::AssetPath;
        using GEF::FileSystem::ReadTextFile;

        const std::string base = "shaders/" + std::string(name);
        const auto vs = ReadTextFile(AssetPath(base + ".vert"));
        const auto fs = ReadTextFile(AssetPath(base + ".frag"));
        if (!vs || !fs)
            return std::nullopt;

        const ShaderHandle shader = device.CreateShader({ *vs, *fs });
        if (!shader.IsValid())
            return std::nullopt;
        return shader;
    }
}

struct Sandbox
{
    float time = 0.0f;

    Camera camera;
    FreeFlyController freeFlyController;
    GEF::Events::EventBus::SubscriberID resizeSubscription{};

    BufferHandle uniformBuffer;
    ShaderHandle meshShader;
    PipelineHandle meshPipeline;
    Mesh mesh;

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

        uniformBuffer = ctx.device.CreateBuffer(
        { .type = BufferType::Uniform,
          .usage = BufferUsage::Dynamic,
          .size = sizeof(FrameData) });

        // Normals shown as colors: the debug view until lighting (GEF-41)
        if (const auto shader = LoadShader(ctx.device, "mesh"))
        {
            meshShader = *shader;
            // Depth test + back-face culling: the faces behind are hidden
            meshPipeline = ctx.device.CreatePipeline(
            { .shader = meshShader,
              .depth = { .test = true, .write = true },
              .cull = CullMode::Back });
        }

        const auto path = GEF::FileSystem::AssetPath(
            "models/stanford-bunny.obj");
        if (const auto data = GEF::Assets::LoadObj(path))
            mesh = UploadMesh(ctx.device, *data);
        else
            GEF_ERROR("Failed to load mesh: {}", path.string());
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
        if (!meshPipeline.IsValid() || mesh.indexCount == 0)
            return;

        cmd.BindPipeline(meshPipeline);
        cmd.BindUniformBuffer(0, uniformBuffer);
        cmd.BindVertexArray(mesh.vertexArray);
        cmd.DrawIndexed(mesh.indexCount);
    }

    void OnShutdown(GEF::EngineContext& ctx)
    {
        ctx.events.Unsubscribe(
            GEF::Events::WindowResizeEvent::GetStaticEventType(),
            resizeSubscription);

        if (mesh.indexCount != 0)
            DestroyMesh(ctx.device, mesh);
        if (meshPipeline.IsValid())
            ctx.device.DestroyPipeline(meshPipeline);
        if (meshShader.IsValid())
            ctx.device.DestroyShader(meshShader);
        ctx.device.DestroyBuffer(uniformBuffer);
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
