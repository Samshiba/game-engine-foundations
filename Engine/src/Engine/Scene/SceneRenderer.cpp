//
// Created by genin on 06/10/2026.
// Path: Engine/src/Engine/Scene/SceneRenderer.cpp
//

#include <ranges>
#include <algorithm>

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
}

GEF::Scene::SceneRenderer::~SceneRenderer()
{
    device_.DestroyPipeline(pipeline_);
    device_.DestroyBuffer(frameUBO_);
    device_.DestroyBuffer(lightUBO_);

    for (const auto& [key, batch] : batches_)
    {
        device_.DestroyVertexArray(batch.vertexArray);
        device_.DestroyBuffer(batch.instanceBuffer);
    }
}

void GEF::Scene::SceneRenderer::Render(
    const libecs::core::registry::Registry& registry,
    const Renderer::Camera& camera, Renderer::CommandList& cmd)
{
    cmd.BindPipeline(pipeline_);
    cmd.BindUniformBuffer(Renderer::UniformBinding::Frame, frameUBO_);
    cmd.BindUniformBuffer(Renderer::UniformBinding::Light, lightUBO_);

    // ----- Light -----
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

    // ----- Frame -----
    const Renderer::FrameUniforms frameU{
        .viewProjection = camera.GetViewProjection(),
        .cameraPosition = glm::vec4(camera.position, 1.0f),
    };
    device_.UpdateBuffer(frameUBO_, 0, &frameU, sizeof(frameU));

    // ----- Instance -----
    // Cleaning batch
    for (auto& batch : batches_ | std::views::values)
    {
        batch.instances.clear();
    }

    // Add batch
    registry.GetView<const Transform, const MeshRenderer>().Each(
        [&](libecs::core::Entity, const Transform& transform,
            const MeshRenderer& meshRenderer) {
            auto mesh = meshRenderer.mesh;

            auto [it, inserted] = batches_.try_emplace(mesh.vertexArray);

            if (inserted)
            {
                it->second = CreateBatch(mesh, InitialBatchCapacity);
            }

            it->second.instances.push_back(
                Renderer::InstanceData{
                    .model = transform.GetMatrix(),
                    .normalMatrix = transform.GetNormalMatrix(),
                    .albedo = glm::pow(meshRenderer.material.albedo,
                                       glm::vec3(2.2f)),
                    .params = glm::vec2(meshRenderer.material.shininess,
                                        meshRenderer.material.specularStrength)
                });
        });

    // Draw
    for (auto& batch : batches_ | std::views::values)
    {
        if (batch.instances.empty())
            continue;

        if (batch.instances.size() > batch.capacity)
        {
            GrowBatch(batch, std::max(
                          static_cast<uint32_t>(batch.instances.size()),
                          batch.capacity * 2));
        }

        device_.UpdateBuffer(batch.instanceBuffer, 0, batch.instances.data(),
                             static_cast<uint32_t>(batch.instances.size()) *
                             sizeof(Renderer::InstanceData));
        cmd.BindVertexArray(batch.vertexArray);
        cmd.DrawIndexedInstanced(batch.mesh.indexCount,
                                 static_cast<uint32_t>(batch.instances.size()));
    }
}

GEF::Scene::SceneRenderer::Batch GEF::Scene::SceneRenderer::CreateBatch(
    const Renderer::Mesh& mesh, uint32_t capacity)
{
    Batch batch;
    batch.mesh = mesh;
    batch.capacity = capacity;
    batch.instanceBuffer = device_.CreateBuffer({
        .type = Renderer::BufferType::Vertex,
        .usage = Renderer::BufferUsage::Dynamic,
        .size = static_cast<uint32_t>(sizeof(Renderer::InstanceData)) * capacity
    });
    batch.vertexArray = device_.CreateVertexArray({
        .vertexBuffers = {
            { .buffer = mesh.vertexBuffer,
              // binding 0: per vertex
              .layout = Renderer::Vertex::Layout() },
            { .buffer = batch.instanceBuffer,
              // binding 1: per instance
              .layout = Renderer::InstanceData::Layout(),
              .instanceDivisor = 1 },
        },
        .indexBuffer = mesh.indexBuffer,
    });
    return batch;
}

void GEF::Scene::SceneRenderer::GrowBatch(Batch& batch, uint32_t capacity)
{
    batch.capacity = capacity;
    device_.DestroyVertexArray(batch.vertexArray);
    device_.DestroyBuffer(batch.instanceBuffer);

    batch.instanceBuffer = device_.CreateBuffer({
        .type = Renderer::BufferType::Vertex,
        .usage = Renderer::BufferUsage::Dynamic,
        .size = static_cast<uint32_t>(sizeof(Renderer::InstanceData)) * capacity
    });
    batch.vertexArray = device_.CreateVertexArray({
        .vertexBuffers = {
            { .buffer = batch.mesh.vertexBuffer,
              // binding 0: per vertex
              .layout = Renderer::Vertex::Layout() },
            { .buffer = batch.instanceBuffer,
              // binding 1: per instance
              .layout = Renderer::InstanceData::Layout(),
              .instanceDivisor = 1 },
        },
        .indexBuffer = batch.mesh.indexBuffer,
    });
}
