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
#include "Engine/Renderer/LightingData.hpp"

using namespace GEF::Renderer;

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

    BufferHandle FrameUBO;
    BufferHandle ObjectUBO;
    BufferHandle LightUBO;
    BufferHandle MaterialUBO;

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

        FrameUBO = ctx.device.CreateBuffer(
        { .type = BufferType::Uniform,
          .usage = BufferUsage::Dynamic,
          .size = sizeof(FrameUniforms) });

        LightUBO = ctx.device.CreateBuffer(
        { .type = BufferType::Uniform,
          .usage = BufferUsage::Dynamic,
          .size = sizeof(LightUniforms) });

        ObjectUBO = ctx.device.CreateBuffer(
        { .type = BufferType::Uniform,
          .usage = BufferUsage::Dynamic,
          .size = sizeof(ObjectUniforms) });

        MaterialUBO = ctx.device.CreateBuffer(
        { .type = BufferType::Uniform,
          .usage = BufferUsage::Dynamic,
          .size = sizeof(MaterialUniforms) });

        if (const auto shader = LoadShader(ctx.device, "lit"))
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

        // The Stanford Bunny is ~0.15 units tall, sitting around (-0.017, 0.11, 0):
        // center it on the origin, then scale it to ~1.5 units
        const glm::vec3 bunnyCenter(-0.017f, 0.11f, -0.0015f);
        const glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f))
            * glm::translate(glm::mat4(1.0f), -bunnyCenter);
        const ObjectUniforms ObjectU{
            .model = model,
            .normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)))
        };

        const MaterialUniforms MaterialU{
            .albedo = glm::vec4(glm::pow(glm::vec3(0.5f), glm::vec3(2.2f)),
                                1.0f),
            .params = glm::vec4(32.0f, 1.0f, 0.0f, 0.0f)
        };

        ctx.device.UpdateBuffer(ObjectUBO, 0, &ObjectU, sizeof(ObjectU));
        ctx.device.UpdateBuffer(MaterialUBO, 0, &MaterialU, sizeof(MaterialU));
    }

    void OnUpdate(GEF::EngineContext& ctx, float dt)
    {
        time += dt;

        freeFlyController.Update(camera, ctx.input, ctx.window, dt);

        const FrameUniforms frameU{
            .viewProjection = camera.GetViewProjection(),
            .cameraPosition = glm::vec4(camera.position, 1.0f),
        };

        const glm::vec3 direction(glm::cos(time), -0.5f, glm::sin(time));
        // a light turning around the bunny, from above
        const LightUniforms lightU{
            .direction = glm::vec4(glm::normalize(direction), 0.0f),
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .ambient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),

        };

        ctx.device.UpdateBuffer(FrameUBO, 0, &frameU, sizeof(frameU));
        ctx.device.UpdateBuffer(LightUBO, 0, &lightU, sizeof(lightU));
    }

    void OnRender(GEF::EngineContext&, CommandList& cmd)
    {
        if (!meshPipeline.IsValid() || mesh.indexCount == 0)
            return;

        cmd.BindPipeline(meshPipeline);
        cmd.BindUniformBuffer(UniformBinding::Frame, FrameUBO);
        cmd.BindUniformBuffer(UniformBinding::Object, ObjectUBO);
        cmd.BindUniformBuffer(UniformBinding::Light, LightUBO);
        cmd.BindUniformBuffer(UniformBinding::Material, MaterialUBO);
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
        ctx.device.DestroyBuffer(FrameUBO);
        ctx.device.DestroyBuffer(LightUBO);
        ctx.device.DestroyBuffer(ObjectUBO);
        ctx.device.DestroyBuffer(MaterialUBO);
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
