//
// Created by genin on 19/11/2026.
// Path: Application/src/StressTestScene.cpp
//

#include "StressTestScene.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

#include <Engine/Scene/Components.hpp>

using Registry = libecs::core::registry::Registry;

namespace
{
    struct Spin
    {
        float speed = 1.0f;
    };
} // namespace

void StressTestScene::Load(GEF::Renderer::GraphicsDevice& device)
{
    cube_ = LoadModel(device, "cube.obj", "MetalPlatesTexture.jpg",
                      "MetalPlatesNormal.jpg");
}

void StressTestScene::Unload(GEF::Renderer::GraphicsDevice& device)
{
    DestroyModel(device, cube_);
}

void StressTestScene::Spawn(Registry& registry)
{
    directionalLight_ = registry.CreateEntity();
    registry.EmplaceComponent<GEF::Scene::DirectionalLight>(
        directionalLight_, GEF::Scene::DirectionalLight{});
    registry.EmplaceComponent<GEF::Scene::Transform>(directionalLight_,
                                                     GEF::Scene::Transform{});

    pointLight_ = registry.CreateEntity();
    registry.EmplaceComponent<GEF::Scene::PointLight>(
        pointLight_,
        GEF::Scene::PointLight{ .color = { 1.0f, 0.0f, 0.0f },
                                .intensity = 200.0f,
                                .range = 200.0f });
    registry.EmplaceComponent<GEF::Scene::Transform>(
        pointLight_,
        GEF::Scene::Transform{ .position = { 30.0f, 10.0f, 50.0f } });

    UpdateEntityCount(registry);
}

void StressTestScene::Despawn(Registry& registry)
{
    for (const auto cube : cubes_)
        registry.DestroyEntity(cube);
    cubes_.clear();

    registry.DestroyEntity(directionalLight_);
    registry.DestroyEntity(pointLight_);
}

void StressTestScene::SpawnCube(Registry& registry, int index)
{
    const int x = index / 250;
    const int z = index % 250;

    const auto cube = registry.CreateEntity();
    cubes_.emplace_back(cube);

    registry.EmplaceComponent<GEF::Scene::Transform>(
        cube,
        GEF::Scene::Transform{
            .position = { (x - 5) * 5.0f, 0.0f, (z - 5) * 5.0f },
            .scale = glm::vec3(10.0f) });

    const auto material = GEF::Scene::Material{ .albedo = { 1.0f, 1.0f, 1.0f },
                                                .shininess = 32.0f,
                                                .specularStrength = 1.0f,
                                                .albedoMap = cube_.albedoMap,
                                                .normalMap = cube_.normalMap };
    registry.EmplaceComponent<GEF::Scene::MeshRenderer>(
        cube,
        GEF::Scene::MeshRenderer{ .mesh = cube_.mesh, .material = material });

    if ((x + z) % 2 == 0)
        registry.EmplaceComponent<Spin>(cube, Spin{ .speed = 1.0f });
}

void StressTestScene::UpdateEntityCount(Registry& registry)
{
    const auto target = static_cast<size_t>(entityCount_);

    while (cubes_.size() < target)
        SpawnCube(registry, static_cast<int>(cubes_.size()));

    while (cubes_.size() > target)
    {
        registry.DestroyEntity(cubes_.back());
        cubes_.pop_back();
    }
}

void StressTestScene::OnUpdate(Registry& registry, float dt)
{
    UpdateEntityCount(registry);

    registry.GetView<Spin, GEF::Scene::Transform>().Each(
        [&](libecs::core::Entity, Spin& spin,
            GEF::Scene::Transform& transform) {
            transform.rotation =
                glm::rotate(transform.rotation, spin.speed * dt,
                            glm::vec3(0.0f, 1.0f, 0.0f));
        });

    registry.GetComponent<GEF::Scene::Transform>(directionalLight_).rotation =
        glm::angleAxis(glm::radians(azimuth_), glm::vec3(0, 1, 0))
        * glm::angleAxis(glm::radians(-elevation_), glm::vec3(1, 0, 0));
}

void StressTestScene::OnImGui(Registry& registry)
{
    ImGui::Text("Entities: %zu", cubes_.size());
    ImGui::SliderInt("Entities", &entityCount_, 0, 50000);

    ImGui::SeparatorText("Lights");
    ImGui::SliderFloat("Sun Elevation", &elevation_, -10.0f, 90.0f);
    ImGui::SliderFloat("Sun Azimuth", &azimuth_, 0.0f, 360.0f);

    auto& sun =
        registry.GetComponent<GEF::Scene::DirectionalLight>(directionalLight_);
    ImGui::SliderFloat("Sun Intensity", &sun.intensity, 0.0f, 20.0f);

    auto& point = registry.GetComponent<GEF::Scene::PointLight>(pointLight_);
    ImGui::SliderFloat("Point Light Intensity", &point.intensity, 0.0f,
                       2000.0f);
    ImGui::ColorEdit3("Point Light Color", &point.color.x);
}

GEF::Renderer::Camera StressTestScene::DefaultCamera() const
{
    return GEF::Renderer::Camera{};
}

const GEF::Scene::SceneRenderer::Environment&
StressTestScene::GetEnvironment() const
{
    return environment_;
}
