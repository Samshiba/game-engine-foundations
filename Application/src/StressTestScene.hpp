//
// Created by genin on 19/11/2026.
// Path: Application/src/StressTestScene.hpp
//

#pragma once

#include <vector>

#include <libecs/core/registry/Registry.hpp>

#include <Engine/Renderer/Camera.hpp>
#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Scene/SceneRenderer.hpp>

#include "Model.hpp"

// Up to 50,000 textured cubes, half of them spinning
class StressTestScene
{
public:
    void Load(GEF::Renderer::GraphicsDevice& device);
    void Unload(GEF::Renderer::GraphicsDevice& device);

    void Spawn(libecs::core::registry::Registry& registry);
    void Despawn(libecs::core::registry::Registry& registry);

    void OnUpdate(libecs::core::registry::Registry& registry, float dt);
    void OnImGui(libecs::core::registry::Registry& registry);

    [[nodiscard]] GEF::Renderer::Camera DefaultCamera() const;
    [[nodiscard]] const GEF::Scene::SceneRenderer::Environment&
    GetEnvironment() const;

private:
    void SpawnCube(libecs::core::registry::Registry& registry, int index);
    void UpdateEntityCount(libecs::core::registry::Registry& registry);

private:
    Model cube_;

    int entityCount_ = 10000;
    std::vector<libecs::core::Entity> cubes_;

    libecs::core::Entity directionalLight_{};
    libecs::core::Entity pointLight_{};
    float elevation_ = 45.0f;
    float azimuth_ = 30.0f;

    GEF::Scene::SceneRenderer::Environment environment_{
        .ambient = { 0.1f, 0.1f, 0.1f }
    };
};
