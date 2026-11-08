//
// Created by genin on 06/10/2026.
// Path: Engine/src/Engine/Scene/SceneRenderer.cpp
//

#include <Engine/Scene/SceneRenderer.hpp>

#include <algorithm>
#include <ranges>

#include <../../../include/Engine/Renderer/Layout/LitShaderLayout.hpp>

#include <Engine/Core/Assert.hpp>
#include <Engine/Core/Embedded.hpp>
#include <Engine/Scene/Components.hpp>

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
          .size = sizeof(Renderer::Layout::FrameUniforms) });

    lightUBO_ = device.CreateBuffer(
        { .type = Renderer::BufferType::Uniform,
          .usage = Renderer::BufferUsage::Dynamic,
          .size = sizeof(Renderer::Layout::LightUniforms) });

    environmentUBO_ = device.CreateBuffer(
        { .type = Renderer::BufferType::Uniform,
          .usage = Renderer::BufferUsage::Dynamic,
          .size = sizeof(Renderer::Layout::EnvironmentUniforms) });
}

GEF::Scene::SceneRenderer::SceneRenderer(Renderer::GraphicsDevice& device)
    : device_(device)
{
    const Renderer::ShaderHandle shader = device.CreateShader(
        { Embedded::LitVertexSource(), Embedded::LitFragmentSource() });
    GEF_CORE_ASSERT(shader.IsValid(), "Failed to create shader");

    litShader_ = shader;

    pipeline_ = device_.CreatePipeline({ .shader = shader,
                                         .depth = { true, true },
                                         .cull = Renderer::CullMode::Back });
    frameUBO_ = device.CreateBuffer(
        { .type = Renderer::BufferType::Uniform,
          .usage = Renderer::BufferUsage::Dynamic,
          .size = sizeof(Renderer::Layout::FrameUniforms) });

    lightUBO_ = device.CreateBuffer(
        { .type = Renderer::BufferType::Uniform,
          .usage = Renderer::BufferUsage::Dynamic,
          .size = sizeof(Renderer::Layout::LightUniforms) });

    environmentUBO_ = device.CreateBuffer(
        { .type = Renderer::BufferType::Uniform,
          .usage = Renderer::BufferUsage::Dynamic,
          .size = sizeof(Renderer::Layout::EnvironmentUniforms) });
}

GEF::Scene::SceneRenderer::~SceneRenderer()
{
    device_.DestroyPipeline(pipeline_);
    device_.DestroyBuffer(frameUBO_);
    device_.DestroyBuffer(lightUBO_);
    device_.DestroyBuffer(environmentUBO_);

    for (const auto& [key, batch] : batches_)
    {
        device_.DestroyVertexArray(batch.vertexArray);
        device_.DestroyBuffer(batch.instanceBuffer);
    }

    if (litShader_.IsValid())
    {
        device_.DestroyShader(litShader_);
    }
}

void GEF::Scene::SceneRenderer::Render(
    const libecs::core::registry::Registry& registry,
    const Renderer::Camera& camera, const Environment& environment,
    Renderer::CommandList& cmd)
{
    cmd.BindPipeline(pipeline_);
    cmd.BindUniformBuffer(Renderer::Layout::UniformBinding::Frame, frameUBO_);
    cmd.BindUniformBuffer(Renderer::Layout::UniformBinding::Light, lightUBO_);
    cmd.BindUniformBuffer(Renderer::Layout::UniformBinding::Environment,
                          environmentUBO_);

    // ----- Environment -----
    const Renderer::Layout::EnvironmentUniforms envU{
        .ambient = glm::vec4(environment.ambient, 0.0f)
    };
    device_.UpdateBuffer(environmentUBO_, 0, &envU, sizeof(envU));

    // ----- Light -----
    DirectionalLight light;
    registry.GetView<const DirectionalLight>().Each(
        [&](libecs::core::Entity, const DirectionalLight& directionalLight) {
            light = directionalLight;
        });
    const Renderer::Layout::LightUniforms lightU{
        .direction = glm::vec4(glm::normalize(light.direction), 1.0f),
        .color = glm::vec4(light.color * light.intensity, 1.0f),
    };
    device_.UpdateBuffer(lightUBO_, 0, &lightU, sizeof(lightU));

    // ----- Frame -----
    const Renderer::Layout::FrameUniforms frameU{
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

            it->second.instances.push_back(Renderer::InstanceData{
                .model = transform.GetMatrix(),
                .normalMatrix = transform.GetNormalMatrix(),
                .albedo =
                    glm::pow(meshRenderer.material.albedo, glm::vec3(2.2f)),
                .params = glm::vec2(meshRenderer.material.shininess,
                                    meshRenderer.material.specularStrength) });
        });

    // Draw
    for (auto& batch : batches_ | std::views::values)
    {
        if (batch.instances.empty())
            continue;

        const auto count = static_cast<uint32_t>(batch.instances.size());

        if (batch.instances.size() > batch.capacity)
        {
            GrowBatch(batch, std::max(count, batch.capacity * 2));
        }

        device_.UpdateBuffer(batch.instanceBuffer, 0, batch.instances.data(),
                             count * sizeof(Renderer::InstanceData));

        cmd.BindVertexArray(batch.vertexArray);
        if (instancing_)
        {
            cmd.DrawIndexedInstanced(batch.mesh.indexCount, count, 0);
        }
        else
        {
            for (uint32_t i = 0; i < count; ++i)
                cmd.DrawIndexedInstanced(batch.mesh.indexCount, 1, i);
        }
    }
}

GEF::Scene::SceneRenderer::Batch
GEF::Scene::SceneRenderer::CreateBatch(const Renderer::Mesh& mesh,
                                       uint32_t capacity)
{
    Batch batch;
    batch.mesh = mesh;
    batch.capacity = capacity;
    batch.instanceBuffer = device_.CreateBuffer(
        { .type = Renderer::BufferType::Vertex,
          .usage = Renderer::BufferUsage::Dynamic,
          .size = static_cast<uint32_t>(sizeof(Renderer::InstanceData))
              * capacity });
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

    batch.instanceBuffer = device_.CreateBuffer(
        { .type = Renderer::BufferType::Vertex,
          .usage = Renderer::BufferUsage::Dynamic,
          .size = static_cast<uint32_t>(sizeof(Renderer::InstanceData))
              * capacity });
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

void GEF::Scene::SceneRenderer::SetInstancingEnabled(bool enabled)
{
    instancing_ = enabled;
}

bool GEF::Scene::SceneRenderer::IsInstancingEnabled() const
{
    return instancing_;
}