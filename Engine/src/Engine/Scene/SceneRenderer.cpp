//
// Created by genin on 06/10/2026.
// Path: Engine/src/Engine/Scene/SceneRenderer.cpp
//

#include <Engine/Scene/SceneRenderer.hpp>

#include <algorithm>
#include <ranges>

#include <Engine/Core/Assert.hpp>
#include <Engine/Core/Embedded.hpp>
#include <Engine/Renderer/Layout/LitShaderLayout.hpp>
#include <Engine/Scene/Components.hpp>

namespace
{
    // Lights point along their local -Z (convention)
    glm::vec3 Forward(const GEF::Scene::Transform& transform)
    {
        return glm::normalize(transform.rotation
                              * glm::vec3(0.0f, 0.0f, -1.0f));
    }
} // namespace

GEF::Scene::SceneRenderer::SceneRenderer(Renderer::GraphicsDevice& device,
                                         Renderer::ShaderHandle litShader)
    : device_(device)
{
    CreateResources(litShader);
}

GEF::Scene::SceneRenderer::SceneRenderer(Renderer::GraphicsDevice& device)
    : device_(device)
{
    litShader_ = device.CreateShader(
        { Embedded::LitVertexSource(), Embedded::LitFragmentSource() });
    GEF_CORE_ASSERT(litShader_.IsValid(), "Failed to create shader");

    CreateResources(litShader_);
}

GEF::Scene::SceneRenderer::~SceneRenderer()
{
    device_.DestroyPipeline(pipeline_);
    device_.DestroyBuffer(frameUBO_);
    device_.DestroyBuffer(environmentUBO_);
    device_.DestroyBuffer(directionalLightSSBO_.handle);
    device_.DestroyBuffer(pointLightSSBO_.handle);
    device_.DestroyBuffer(spotLightSSBO_.handle);

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
    cmd.BindUniformBuffer(Renderer::Layout::UniformBinding::Environment,
                          environmentUBO_);

    // ----- Environment -----
    const Renderer::Layout::EnvironmentUniforms envU{
        .ambient = glm::vec4(environment.ambient, 0.0f)
    };
    device_.UpdateBuffer(environmentUBO_, 0, &envU, sizeof(envU));

    // ----- Lights -----
    // collect -> (grow) -> update -> bind
    directionalLights_.clear();
    registry.GetView<const Transform, const DirectionalLight>().Each(
        [&](libecs::core::Entity, const Transform& transform,
            const DirectionalLight& light) {
            directionalLights_.push_back({
                .direction = glm::vec4(Forward(transform), 0.0f),
                .color = glm::vec4(light.color * light.intensity, 0.0f),
            });
        });

    pointLights_.clear();
    registry.GetView<const Transform, const PointLight>().Each(
        [&](libecs::core::Entity, const Transform& transform,
            const PointLight& light) {
            pointLights_.push_back({
                .positionRange = glm::vec4(transform.position, light.range),
                .color = glm::vec4(light.color * light.intensity, 0.0f),
            });
        });

    spotLights_.clear();
    registry.GetView<const Transform, const SpotLight>().Each(
        [&](libecs::core::Entity, const Transform& transform,
            const SpotLight& light) {
            GEF_CORE_ASSERT(
                light.innerConeAngle < light.outerConeAngle,
                "Inner cone angle must be less than outer cone angle");
            const float inner = light.innerConeAngle;
            const float outer = glm::max(light.outerConeAngle, inner + 0.01f);
            spotLights_.push_back({
                .positionRange = glm::vec4(transform.position, light.range),
                .direction = glm::vec4(Forward(transform), 0.0f),
                .color = glm::vec4(light.color * light.intensity, 0.0f),
                .coneCos = glm::vec4(glm::cos(glm::radians(inner)),
                                     glm::cos(glm::radians(outer)), 0.0f, 0.0f),
            });
        });

    UploadLights(directionalLightSSBO_, directionalLights_,
                 Renderer::Layout::ShaderStorageBinding::DirectionalLight, cmd);
    UploadLights(pointLightSSBO_, pointLights_,
                 Renderer::Layout::ShaderStorageBinding::PointLight, cmd);
    UploadLights(spotLightSSBO_, spotLights_,
                 Renderer::Layout::ShaderStorageBinding::SpotLight, cmd);

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

            it->second.instances.push_back(Renderer::Layout::InstanceData{
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
                             count * sizeof(Renderer::Layout::InstanceData));

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

void GEF::Scene::SceneRenderer::CreateResources(
    Renderer::ShaderHandle litShader)
{
    pipeline_ = device_.CreatePipeline({ .shader = litShader,
                                         .depth = { true, true },
                                         .cull = Renderer::CullMode::Back });

    frameUBO_ = device_.CreateBuffer(
        { .type = Renderer::BufferType::Uniform,
          .usage = Renderer::BufferUsage::Dynamic,
          .size = sizeof(Renderer::Layout::FrameUniforms) });

    environmentUBO_ = device_.CreateBuffer(
        { .type = Renderer::BufferType::Uniform,
          .usage = Renderer::BufferUsage::Dynamic,
          .size = sizeof(Renderer::Layout::EnvironmentUniforms) });

    directionalLightSSBO_ =
        CreateLightBuffer<Renderer::Layout::GpuDirectionalLight>(
            InitialLightCapacity);
    pointLightSSBO_ = CreateLightBuffer<Renderer::Layout::GpuPointLight>(
        InitialLightCapacity);
    spotLightSSBO_ =
        CreateLightBuffer<Renderer::Layout::GpuSpotLight>(InitialLightCapacity);
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
          .size = static_cast<uint32_t>(sizeof(Renderer::Layout::InstanceData))
              * capacity });
    batch.vertexArray = device_.CreateVertexArray({
        .vertexBuffers = {
            { .buffer = mesh.vertexBuffer,
              // binding 0: per vertex
              .layout = Renderer::Vertex::Layout() },
            { .buffer = batch.instanceBuffer,
              // binding 1: per instance
              .layout = Renderer::Layout::InstanceData::Layout(),
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
          .size = static_cast<uint32_t>(sizeof(Renderer::Layout::InstanceData))
              * capacity });
    batch.vertexArray = device_.CreateVertexArray({
        .vertexBuffers = {
            { .buffer = batch.mesh.vertexBuffer,
              // binding 0: per vertex
              .layout = Renderer::Vertex::Layout() },
            { .buffer = batch.instanceBuffer,
              // binding 1: per instance
              .layout = Renderer::Layout::InstanceData::Layout(),
              .instanceDivisor = 1 },
        },
        .indexBuffer = batch.mesh.indexBuffer,
    });
}

template <typename GpuLight>
GEF::Scene::SceneRenderer::LightBuffer
GEF::Scene::SceneRenderer::CreateLightBuffer(uint32_t capacity)
{
    return LightBuffer{
        .handle =
            device_.CreateBuffer({ .type = Renderer::BufferType::ShaderStorage,
                                   .usage = Renderer::BufferUsage::Dynamic,
                                   .size = static_cast<uint32_t>(
                                       sizeof(Renderer::Layout::LightListHeader)
                                       + sizeof(GpuLight) * capacity) }),
        .capacity = capacity,
    };
}

template <typename GpuLight>
void GEF::Scene::SceneRenderer::UploadLights(
    LightBuffer& buffer, const std::vector<GpuLight>& lights,
    uint32_t bindingPoint, Renderer::CommandList& cmd)
{
    const auto count = static_cast<uint32_t>(lights.size());

    if (count > buffer.capacity)
    {
        device_.DestroyBuffer(buffer.handle);
        buffer =
            CreateLightBuffer<GpuLight>(std::max(count, buffer.capacity * 2));
    }

    const Renderer::Layout::LightListHeader header{ .count = count, .pad = {} };
    device_.UpdateBuffer(buffer.handle, 0, &header, sizeof(header));
    if (count > 0)
    {
        device_.UpdateBuffer(buffer.handle, sizeof(header), lights.data(),
                             count * static_cast<uint32_t>(sizeof(GpuLight)));
    }

    cmd.BindShaderStorageBuffer(bindingPoint, buffer.handle);
}

void GEF::Scene::SceneRenderer::SetInstancingEnabled(bool enabled)
{
    instancing_ = enabled;
}

bool GEF::Scene::SceneRenderer::IsInstancingEnabled() const
{
    return instancing_;
}