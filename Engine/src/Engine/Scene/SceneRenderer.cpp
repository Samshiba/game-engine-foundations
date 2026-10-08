//
// Created by genin on 06/10/2026.
// Path: Engine/src/Engine/Scene/SceneRenderer.cpp
//

#include <Engine/Scene/SceneRenderer.hpp>
#include <Engine/Scene/Components.hpp>
#include <Engine/Renderer/LightingData.hpp>

GEF::Scene::SceneRenderer::SceneRenderer(Renderer::GraphicsDevice& device,
                                         Renderer::ShaderHandle litShader)
    : device_(device)
{
    pipeline_ = device_.CreatePipeline({ .shader = litShader,
                                         .depth = { true, true },
                                         .cull = Renderer::CullMode::Back });
    frameUBO_ = device.CreateBuffer(
    { .type = Renderer::BufferType::Uniform,
      .usage = Renderer::BufferUsage::Dynamic,
      .size = sizeof(Renderer::FrameUniforms) });

    lightUBO_ = device.CreateBuffer(
    { .type = Renderer::BufferType::Uniform,
      .usage = Renderer::BufferUsage::Dynamic,
      .size = sizeof(Renderer::LightUniforms) });

    objectUBO_ = device.CreateBuffer(
    { .type = Renderer::BufferType::Uniform,
      .usage = Renderer::BufferUsage::Dynamic,
      .size = sizeof(Renderer::ObjectUniforms) });

    materialUBO_ = device.CreateBuffer(
    { .type = Renderer::BufferType::Uniform,
      .usage = Renderer::BufferUsage::Dynamic,
      .size = sizeof(Renderer::MaterialUniforms) });
}

GEF::Scene::SceneRenderer::~SceneRenderer()
{
    device_.DestroyPipeline(pipeline_);
    device_.DestroyBuffer(frameUBO_);
    device_.DestroyBuffer(lightUBO_);
    device_.DestroyBuffer(objectUBO_);
    device_.DestroyBuffer(materialUBO_);
}

void GEF::Scene::SceneRenderer::Render(
    const libecs::core::registry::Registry& registry,
    const Renderer::Camera& camera, Renderer::CommandList& cmd)
{
    cmd.BindPipeline(pipeline_);
    cmd.BindUniformBuffer(Renderer::UniformBinding::Frame, frameUBO_);
    cmd.BindUniformBuffer(Renderer::UniformBinding::Light, lightUBO_);
    cmd.BindUniformBuffer(Renderer::UniformBinding::Object, objectUBO_);
    cmd.BindUniformBuffer(Renderer::UniformBinding::Material, materialUBO_);

    // Light
    DirectionalLight light;
    registry.GetView<const DirectionalLight>().Each(
        [&](libecs::core::Entity, const DirectionalLight& directionalLight) {
            light = directionalLight;
        });
    const Renderer::LightUniforms lightU{
        .direction = glm::vec4(glm::normalize(light.direction), 1.0f),
        .color = glm::vec4(light.color * light.intensity, 1.0f),
        .ambient = glm::vec4(light.ambient, 1.0f),
    };
    device_.UpdateBuffer(lightUBO_, 0, &lightU, sizeof(lightU));

    // Frame
    const Renderer::FrameUniforms frameU{
        .viewProjection = camera.GetViewProjection(),
        .cameraPosition = glm::vec4(camera.position, 1.0f),
    };
    device_.UpdateBuffer(frameUBO_, 0, &frameU, sizeof(frameU));

    // Object & Mesh
    registry.GetView<const Transform, const MeshRenderer>().Each(
        [&](libecs::core::Entity, const Transform& transform,
            const MeshRenderer& meshRenderer) {
            const glm::mat4 model = transform.GetMatrix();
            const Renderer::ObjectUniforms object{
                .model = model,
                .normalMatrix = glm::transpose(
                    glm::inverse(glm::mat3(model))) };
            const Renderer::MaterialUniforms material{
                .albedo = glm::vec4(
                    glm::pow(meshRenderer.material.albedo, glm::vec3(2.2f)),
                    1.0f),
                .params = glm::vec4(meshRenderer.material.shininess,
                                    meshRenderer.material.specularStrength,
                                    0.0f, 0.0f)
            };
            device_.UpdateBuffer(objectUBO_, 0, &object, sizeof(object));
            device_.UpdateBuffer(materialUBO_, 0, &material, sizeof(material));
            cmd.BindVertexArray(meshRenderer.mesh.vertexArray);
            cmd.DrawIndexed(meshRenderer.mesh.indexCount);
        });
}
