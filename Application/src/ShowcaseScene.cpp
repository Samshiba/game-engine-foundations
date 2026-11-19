//
// Created by genin on 19/11/2026.
// Path: Application/src/ShowcaseScene.cpp
//

#include "ShowcaseScene.hpp"

#include <format>

#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

#include <Engine/Scene/Components.hpp>

using Registry = libecs::core::registry::Registry;

void ShowcaseScene::Load(GEF::Renderer::GraphicsDevice& device)
{
    floor_ = LoadModel(device, "floor.obj", "PavingStonesTexture.jpg",
                       "PavingStonesNormal.jpg");
    crate_ = LoadModel(device, "wooden-crate.obj", "WoodenCrateTexture.jpg",
                       "WoodenCrateNormal.jpg");
    bust_ = LoadModel(device, "marble-bust.obj", "MarbleBustTexture.jpg",
                      "MarbleBustNormal.jpg");
    barrel_ = LoadModel(device, "wine-barrel.obj", "WineBarrelTexture.jpg",
                        "WineBarrelNormal.jpg");
    cat_ = LoadModel(device, "concrete-cat.obj", "ConcreteCatTexture.jpg",
                     "ConcreteCatNormal.jpg");
}

void ShowcaseScene::Unload(GEF::Renderer::GraphicsDevice& device)
{
    for (const auto model : { &floor_, &crate_, &bust_, &barrel_, &cat_ })
        DestroyModel(device, *model);
}

libecs::core::Entity ShowcaseScene::SpawnProp(Registry& registry,
                                              const Model& model,
                                              glm::vec3 position,
                                              float yawDegrees, float shininess,
                                              float specularStrength)
{
    const auto entity = registry.CreateEntity();
    props_.push_back(entity);

    registry.EmplaceComponent<GEF::Scene::Transform>(
        entity,
        GEF::Scene::Transform{
            .position = position,
            .rotation = glm::angleAxis(glm::radians(yawDegrees),
                                       glm::vec3(0.0f, 1.0f, 0.0f)) });
    registry.EmplaceComponent<GEF::Scene::MeshRenderer>(
        entity,
        GEF::Scene::MeshRenderer{
            .mesh = model.mesh,
            .material = { .albedo = glm::vec3(1.0f),
                          .shininess = shininess,
                          .specularStrength = specularStrength,
                          .albedoMap = model.albedoMap,
                          .normalMap = model.normalMap } });
    return entity;
}

libecs::core::Entity ShowcaseScene::SpawnPointLight(Registry& registry,
                                                    glm::vec3 position,
                                                    glm::vec3 color,
                                                    float intensity)
{
    const auto entity = registry.CreateEntity();
    registry.EmplaceComponent<GEF::Scene::PointLight>(
        entity,
        GEF::Scene::PointLight{
            .color = color, .intensity = intensity, .range = 3.0f });
    registry.EmplaceComponent<GEF::Scene::Transform>(
        entity, GEF::Scene::Transform{ .position = position });
    return entity;
}

void ShowcaseScene::Spawn(Registry& registry)
{
    const auto floor =
        SpawnProp(registry, floor_, { 0.0f, 0.0f, 0.0f }, 0.0f, 24.0f, 0.1f);
    registry.GetComponent<GEF::Scene::MeshRenderer>(floor).material.albedo =
        glm::vec3(0.45f);
    SpawnProp(registry, crate_, { 0.0f, 0.0f, 0.0f }, 90.0f, 16.0f, 0.15f);
    SpawnProp(registry, bust_, { 0.0f, 0.455f, 0.0f }, 0.0f, 64.0f, 0.6f);
    SpawnProp(registry, barrel_, { -0.75f, 0.0f, -0.3f }, 20.0f, 32.0f, 0.3f);
    SpawnProp(registry, cat_, { 0.7f, 0.0f, 0.2f }, -30.0f, 16.0f, 0.1f);

    sun_ = registry.CreateEntity();
    registry.EmplaceComponent<GEF::Scene::DirectionalLight>(
        sun_,
        GEF::Scene::DirectionalLight{ .color = { 0.9f, 0.95f, 1.0f },
                                      .intensity = 0.02f });
    registry.EmplaceComponent<GEF::Scene::Transform>(
        sun_,
        GEF::Scene::Transform{
            .rotation = glm::angleAxis(glm::radians(-60.0f),
                                       glm::vec3(1.0f, 0.0f, 0.0f)) });

    pointLights_[0] = SpawnPointLight(registry, { -1.0f, 0.7f, 0.5f },
                                      { 1.0f, 0.45f, 0.1f }, 6.0f);
    pointLights_[1] = SpawnPointLight(registry, { 1.0f, 0.8f, 0.05f },
                                      { 0.15f, 0.4f, 1.0f }, 6.0f);
    pointLights_[2] = SpawnPointLight(registry, { 0.25f, 1.2f, -0.8f },
                                      { 1.0f, 0.15f, 0.7f }, 5.0f);
}

void ShowcaseScene::Despawn(Registry& registry)
{
    for (const auto entity : props_)
        registry.DestroyEntity(entity);
    props_.clear();

    registry.DestroyEntity(sun_);
    for (const auto light : pointLights_)
        registry.DestroyEntity(light);
}

void ShowcaseScene::OnUpdate(Registry& registry, float dt)
{
    if (!animateLights_)
        return;

    const auto rotation =
        glm::angleAxis(0.25f * dt, glm::vec3(0.0f, 1.0f, 0.0f));
    for (const auto light : pointLights_)
    {
        auto& transform = registry.GetComponent<GEF::Scene::Transform>(light);
        transform.position = rotation * transform.position;
    }
}

void ShowcaseScene::OnImGui(Registry& registry)
{
    ImGui::Checkbox("Animate lights", &animateLights_);
    ImGui::ColorEdit3("Ambient", &environment_.ambient.x);

    auto& sun = registry.GetComponent<GEF::Scene::DirectionalLight>(sun_);
    ImGui::SliderFloat("Sun Intensity", &sun.intensity, 0.0f, 5.0f);

    for (size_t i = 0; i < pointLights_.size(); ++i)
    {
        auto& light =
            registry.GetComponent<GEF::Scene::PointLight>(pointLights_[i]);
        ImGui::PushID(static_cast<int>(i));
        ImGui::SeparatorText(std::format("Point light {}", i + 1).c_str());
        ImGui::ColorEdit3("Color", &light.color.x);
        ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 20.0f);
        ImGui::SliderFloat("Range", &light.range, 0.1f, 20.0f);
        ImGui::PopID();
    }
}

GEF::Renderer::Camera ShowcaseScene::DefaultCamera() const
{
    return GEF::Renderer::Camera{ .position = { 0.05f, 0.85f, 1.4f },
                                  .yaw = -92.0f,
                                  .pitch = -11.0f };
}

const GEF::Scene::SceneRenderer::Environment&
ShowcaseScene::GetEnvironment() const
{
    return environment_;
}
