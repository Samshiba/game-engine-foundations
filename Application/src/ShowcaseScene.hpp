//
// Created by genin on 19/11/2026.
// Path: Application/src/ShowcaseScene.hpp
//

#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <libecs/core/registry/Registry.hpp>

#include <Engine/Renderer/Camera.hpp>
#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Scene/SceneRenderer.hpp>

#include "Model.hpp"

class ShowcaseScene
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
    libecs::core::Entity SpawnProp(libecs::core::registry::Registry& registry,
                                   const Model& model, glm::vec3 position,
                                   float yawDegrees, float shininess,
                                   float specularStrength);
    libecs::core::Entity
    SpawnPointLight(libecs::core::registry::Registry& registry,
                    glm::vec3 position, glm::vec3 color, float intensity);

private:
    Model floor_;
    Model crate_;
    Model bust_;
    Model barrel_;
    Model cat_;

    std::vector<libecs::core::Entity> props_;
    libecs::core::Entity sun_{};
    std::array<libecs::core::Entity, 3> pointLights_{};

    bool animateLights_ = false;

    GEF::Scene::SceneRenderer::Environment environment_{
        .ambient = { 0.03f, 0.03f, 0.04f }
    };
};
