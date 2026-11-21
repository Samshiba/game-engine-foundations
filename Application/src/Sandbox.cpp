//
// Created by genin on 26/01/2026.
// Path: Application/src/Sandbox.cpp
//

#include <optional>

#include <imgui.h>
#include <libecs/core/registry/Registry.hpp>

#include <Engine/Core/DedicatedGpu.hpp>
#include <Engine/Core/Engine.hpp>
#include <Engine/Core/Log.hpp>
#include <Engine/Event/ApplicationEvent.hpp>
#include <Engine/Renderer/Camera.hpp>
#include <Engine/Scene/SceneRenderer.hpp>
#include <Engine/Utils/FrameTimer.hpp>

#include "FreeFlyController.hpp"
#include "ShowcaseScene.hpp"
#include "StressTestScene.hpp"

using namespace GEF::Renderer;
using DebugView = GEF::Scene::SceneRenderer::DebugView;

// Laptops: run on the dedicated GPU, not the integrated one
GEF_REQUEST_DEDICATED_GPU();

namespace
{
    enum class SceneId : int
    {
        Showcase,
        StressTest
    };

    constexpr int DebugViewCount =
        static_cast<int>(DebugView::LightingOnly) + 1;
} // namespace

struct Sandbox
{
    libecs::core::registry::Registry registry;

    Camera camera;
    FreeFlyController freeFlyController;
    GEF::Events::EventBus::SubscriberID resizeSubscription{};

    std::optional<GEF::Scene::SceneRenderer> renderer;
    RenderTargetHandle sceneTarget;

    GEF::Utils::FrameTimer frameTimer;

    ShowcaseScene showcase;
    StressTestScene stressTest;
    SceneId activeScene = SceneId::Showcase;

    bool showUi = true;
    int capturedView = -1; // -1 when no F10 capture is running

    RenderTargetHandle shadowTarget;

    void SpawnActiveScene()
    {
        const float aspectRatio = camera.aspectRatio;
        if (activeScene == SceneId::Showcase)
        {
            showcase.Spawn(registry);
            camera = showcase.DefaultCamera();
        }
        else
        {
            stressTest.Spawn(registry);
            camera = stressTest.DefaultCamera();
        }
        camera.aspectRatio = aspectRatio;
    }

    void DespawnActiveScene()
    {
        if (activeScene == SceneId::Showcase)
            showcase.Despawn(registry);
        else
            stressTest.Despawn(registry);
    }

    void OnInit(GEF::EngineContext& ctx)
    {
        camera.aspectRatio =
            float(ctx.window.GetWidth()) / float(ctx.window.GetHeight());

        resizeSubscription = ctx.events.Subscribe(
            GEF::Events::WindowResizeEvent::GetStaticEventType(),
            [this](GEF::Events::Event const& e) {
                const auto& resize =
                    static_cast<GEF::Events::WindowResizeEvent const&>(e);
                if (resize.GetHeight() == 0)
                    return;
                camera.aspectRatio =
                    float(resize.GetWidth()) / float(resize.GetHeight());
            });

        sceneTarget = ctx.device.CreateRenderTarget(
            { .sizeMode = SizeMode::WindowRelative,
              .scaleX = 1.0f,
              .scaleY = 1.0f,
              .attachments = { TextureFormat::RGBA8 },
              .depthStencil = TextureFormat::Depth32F });

        shadowTarget = ctx.device.CreateRenderTarget(
            { .width = 2048,
              .height = 2048,
              .depthStencil = TextureFormat::Depth32F });

        renderer.emplace(ctx.device);

        renderer->SetShadowMap(ctx.device.GetRenderTargetDepth(shadowTarget));

        showcase.Load(ctx.device);
        stressTest.Load(ctx.device);

        SpawnActiveScene();
    }

    // F10: one frame per debug view, same camera, no UI
    void UpdateDebugCapture(GEF::EngineContext& ctx)
    {
        if (capturedView < 0)
        {
            if (!ctx.input.IsKeyJustPressed(GEF::Key::KeyCode::KEY_F10)
                || ctx.capture.IsCapturing())
                return;

            showUi = false;
            capturedView = 0;
            ctx.capture.RequestSequence(DebugViewCount);
        }

        if (capturedView == DebugViewCount)
        {
            renderer->SetDebugView(DebugView::None);
            showUi = true;
            capturedView = -1;
            return;
        }

        renderer->SetDebugView(static_cast<DebugView>(capturedView));
        capturedView++;
    }

    void OnUpdate(GEF::EngineContext& ctx, float dt)
    {
        frameTimer.Update(dt);

        UpdateDebugCapture(ctx);

        if (capturedView < 0)
        {
            if (activeScene == SceneId::Showcase)
                showcase.OnUpdate(registry, dt);
            else
                stressTest.OnUpdate(registry, dt);
        }

        freeFlyController.Update(camera, ctx.input, ctx.window, dt);

        if (ctx.input.IsKeyJustPressed(GEF::Key::KeyCode::KEY_F1))
            showUi = !showUi;
        if (ctx.input.IsKeyJustPressed(GEF::Key::KeyCode::KEY_F12))
            ctx.capture.RequestScreenshot();
        if (ctx.input.IsKeyJustPressed(GEF::Key::KeyCode::KEY_F11))
            ctx.capture.RequestSequence(200);
        if (ctx.input.IsKeyJustPressed(GEF::Key::KeyCode::KEY_R))
        {
            activeScene == SceneId::Showcase
                ? camera = showcase.DefaultCamera()
                : camera = stressTest.DefaultCamera();
        }
    }

    void OnRender(GEF::EngineContext&, CommandList& cmd)
    {
        const auto& environment = activeScene == SceneId::Showcase
            ? showcase.GetEnvironment()
            : stressTest.GetEnvironment();

        cmd.BeginPass({ .target = shadowTarget, .clearColor = false });
        renderer->RenderShadow(registry, camera, environment, cmd);
        cmd.EndPass();

        cmd.BeginPass({ .target = sceneTarget,
                        .clearValue = { 0.01f, 0.01f, 0.012f, 1 } });
        renderer->Render(registry, camera, environment, cmd);
        cmd.EndPass();

        cmd.BeginPass({ .clearColor = false, .clearDepth = false });
        cmd.Blit(sceneTarget);
        cmd.EndPass();
    }

    void OnShutdown(GEF::EngineContext& ctx)
    {
        ctx.events.Unsubscribe(
            GEF::Events::WindowResizeEvent::GetStaticEventType(),
            resizeSubscription);

        DespawnActiveScene();
        renderer.reset();

        showcase.Unload(ctx.device);
        stressTest.Unload(ctx.device);
        ctx.device.DestroyRenderTarget(sceneTarget);
        ctx.device.DestroyRenderTarget(shadowTarget);
    }

    void OnImGui(GEF::EngineContext& ctx)
    {
        if (!showUi)
            return;

        ImGui::Begin("Stats");

        int scene = static_cast<int>(activeScene);
        if (ImGui::Combo("Scene", &scene, "Showcase\0Stress test\0"))
        {
            DespawnActiveScene();
            activeScene = static_cast<SceneId>(scene);
            SpawnActiveScene();
        }
        ImGui::TextDisabled("F1: hide UI  F10: capture debug views");
        ImGui::TextDisabled(
            "F11: capture sequence (gif), F12: capture screenshot ");
        ImGui::TextDisabled("Hold right click: enable free fLy");
        ImGui::TextDisabled("R: reset camera");

        ImGui::SeparatorText("Frame");
        // Frame Infos
        ImGui::Text("GPU: %s", ctx.device.GetAdapterInfo().renderer.c_str());
        ImGui::Text("FPS: %.2f", frameTimer.GetFPS());
        ImGui::Text("Avg Frame Time: %.2f ms",
                    frameTimer.GetAvgFrameTime() * 1000.0f);
        ImGui::Text("p99 Frame Time: %.2f ms",
                    frameTimer.GetStatistics().p99Ms);
        ImGui::Text("Worst Frame Time: %.2f ms",
                    frameTimer.GetWorstFrameTime() * 1000.0f);
        ImGui::Text("GPU Frame Time: %.2f ms",
                    ctx.device.GetLastSubmitStats().gpuFrameTimeMs);
        ImGui::Text("CPU Submit Time: %.2f ms",
                    ctx.device.GetLastSubmitStats().cpuSubmitTimeMs);

        // VSync
        bool vsync = ctx.window.IsVSync();
        if (ImGui::Checkbox("VSync", &vsync))
            ctx.window.SetVSync(vsync);

        // Debug View
        int view = static_cast<int>(renderer->GetDebugView());
        if (ImGui::Combo("Debug View", &view,
                         "None\0Albedo\0Normal\0UV\0Tangent\0Normal "
                         "Map\0Lighting Only\0"))
            renderer->SetDebugView(static_cast<DebugView>(view));

        ImGui::SeparatorText("Draws");
        bool instancing = renderer->IsInstancingEnabled();
        if (ImGui::Checkbox("Instancing", &instancing))
            renderer->SetInstancingEnabled(instancing);
        const auto stats = ctx.device.GetLastSubmitStats();
        ImGui::Text("Draw calls: %u", stats.drawCalls);
        ImGui::Text("Pipeline changes: %u", stats.pipelineChanges);
        ImGui::Text("Vertex Arrays changes: %u", stats.vertexArrayChanges);
        ImGui::Text("Uniform buffer changes: %u", stats.uniformBufferChanges);
        ImGui::Text("Shader Storage buffer changes: %u",
                    stats.shaderStorageBufferChanges);
        ImGui::Text("Texture changes: %u", stats.textureChanges);

        ImGui::SeparatorText("Scene");
        if (activeScene == SceneId::Showcase)
            showcase.OnImGui(registry);
        else
            stressTest.OnImGui(registry);

        ImGui::End();
    }
};

int main()
{
    GEF::Engine engine(
        { .title = "Sandbox",
          .windowPositionConfig = { .type = GEF::Window::WindowInitialPosition::
                                        CENTER_PRIMARY_SCREEN } });
    Sandbox game;
    const int status = engine.Run(game);

    GEF_INFO("Sandbox finished with status: {}", status);
    return status;
}
