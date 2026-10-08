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
#include <Engine/Scene/Components.hpp>
#include <Engine/Scene/SceneRenderer.hpp>

#include <libecs/core/registry/Registry.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include <optional>
#include <string>
#include <string_view>

#include "FreeFlyController.hpp"
#include "Engine/Utils/FrameTimer.hpp"

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

    struct Spin
    {
        float speed = 1.0f;
    };

    void UpdateSpin(libecs::core::registry::Registry& registry, float dt)
    {
        registry.GetView<Spin, GEF::Scene::Transform>().Each(
            [&](libecs::core::Entity, Spin& spin,
                GEF::Scene::Transform& transform) {
                transform.rotation = glm::rotate(
                    transform.rotation, spin.speed * dt,
                    glm::vec3(0.0f, 1.0f, 0.0f));
            });
    }
}

struct Sandbox
{
    float time = 0.0f;

    libecs::core::registry::Registry registry;

    Camera camera;
    FreeFlyController freeFlyController;
    GEF::Events::EventBus::SubscriberID resizeSubscription{};

    ShaderHandle meshShader;
    Mesh mesh;

    std::optional<GEF::Scene::SceneRenderer> renderer;

    GEF::Utils::FrameTimer frameTimer;

    libecs::core::Entity light;

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

        if (const auto shader = LoadShader(ctx.device, "lit"))
        {
            meshShader = *shader;
        }

        const auto path = GEF::FileSystem::AssetPath(
            "models/stanford-bunny.obj");
        if (const auto data = GEF::Assets::LoadObj(path))
            mesh = UploadMesh(ctx.device, *data);
        else
            GEF_ERROR("Failed to load mesh: {}", path.string());

        if (meshShader.IsValid())
            renderer.emplace(ctx.device, meshShader);

        light = registry.CreateEntity();
        registry.EmplaceComponent<GEF::Scene::DirectionalLight>(
            light, GEF::Scene::DirectionalLight{
                .direction = { -1.0f, -1.0f, -0.5f } });

        for (int x = 0; x < 100; ++x)
        {
            for (int z = 0; z < 100; ++z)
            {
                const auto bunny = registry.CreateEntity();
                registry.EmplaceComponent<GEF::Scene::Transform>(bunny,
                    GEF::Scene::Transform{
                        .position = { (x - 2) * 2.0f, 0.0f, (z - 2) * 2.0f },
                        // 2 units apart, centered
                        .scale = glm::vec3(10.0f) });

                const auto material = GEF::Scene::Material{
                    .albedo = { x / 4.0f, z / 4.0f, 0.5f },
                    .shininess = 32.0f,
                    .specularStrength = 1.0f,
                };
                registry.EmplaceComponent<GEF::Scene::MeshRenderer>(
                    bunny, GEF::Scene::MeshRenderer{
                        .mesh = mesh, .material = material });

                if ((x + z) % 2 == 0)
                {
                    registry.EmplaceComponent<Spin>(
                        bunny, Spin{ .speed = 1.0f });
                }
            }
        }
    }

    void OnUpdate(GEF::EngineContext& ctx, float dt)
    {
        time += dt;
        frameTimer.Update(dt);

        UpdateSpin(registry, dt);
        freeFlyController.Update(camera, ctx.input, ctx.window, dt);
    }

    void OnRender(GEF::EngineContext&, CommandList& cmd)
    {
        if (renderer)
            renderer->Render(registry, camera, cmd);
    }

    void OnShutdown(GEF::EngineContext& ctx)
    {
        ctx.events.Unsubscribe(
            GEF::Events::WindowResizeEvent::GetStaticEventType(),
            resizeSubscription);

        renderer.reset();

        if (mesh.indexCount != 0)
            DestroyMesh(ctx.device, mesh);
        if (meshShader.IsValid())
            ctx.device.DestroyShader(meshShader);
    }

    void OnImGui(GEF::EngineContext& ctx)
    {
        ImGui::Begin("Stats");
        ImGui::Text("FPS: %.2f", frameTimer.GetFPS());
        ImGui::Text("Avg Frame Time: %.2f ms",
                    frameTimer.GetAvgFrameTime() * 1000.0f);
        ImGui::Text("Worst Frame Time: %.2f ms",
                    frameTimer.GetWorstFrameTime() * 1000.0f);

        // VSync
        bool vsync = ctx.window.IsVSync();
        if (ImGui::Checkbox("VSync", &vsync))
            ctx.window.SetVSync(vsync);

        // Light
        glm::vec3 direction = registry.GetComponent<
            GEF::Scene::DirectionalLight>(light).direction;
        ImGui::SliderFloat3("Light Direction", glm::value_ptr(direction), -1.0f,
                            1.0f);
        registry.GetComponent<GEF::Scene::DirectionalLight>(light).direction =
            direction;
        ImGui::End();
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
