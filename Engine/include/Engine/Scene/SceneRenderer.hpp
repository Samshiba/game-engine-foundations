//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Scene/SceneRenderer.hpp
//

#pragma once

#include <libecs/core/registry/Registry.hpp>

#include <Engine/Renderer/Camera.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/Layout/LitShaderLayout.hpp>
#include <Engine/Renderer/Mesh.hpp>
#include <Engine/Utils/HashCombine.hpp>

#include "Components.hpp"

namespace GEF::Scene
{
    class SceneRenderer
    {
    public:
        struct Environment
        {
            glm::vec3 ambient{ 0.1f };
        };

    private:
        struct Batch
        {
            Renderer::BufferHandle instanceBuffer;
            Renderer::VertexArrayHandle vertexArray;
            uint32_t capacity = 0; // instances the buffer can hold
            std::vector<Renderer::Layout::InstanceData>
                instances; // filled each frame
            Renderer::Mesh mesh;
        };

        struct BatchKey
        {
            Renderer::VertexArrayHandle mesh;
            Renderer::TextureHandle albedoMap;
            Renderer::TextureHandle normalMap;
            bool operator==(const BatchKey&) const = default;
        };

        struct BatchKeyHasher
        {
            std::size_t operator()(const BatchKey& key) const noexcept;
        };

        struct LightBuffer
        {
            Renderer::BufferHandle handle;
            uint32_t capacity = 0; // lights the buffer can hold
        };

        static constexpr uint32_t InitialBatchCapacity = 256;
        static constexpr uint32_t InitialLightCapacity = 16;

    public:
        SceneRenderer(Renderer::GraphicsDevice& device,
                      Renderer::ShaderHandle litShader);
        explicit SceneRenderer(Renderer::GraphicsDevice& device);
        ~SceneRenderer();

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;

        void Render(const libecs::core::registry::Registry& registry,
                    const Renderer::Camera& camera,
                    const Environment& environment, Renderer::CommandList& cmd);

        void SetInstancingEnabled(bool enabled);
        [[nodiscard]] bool IsInstancingEnabled() const;

    private:
        void CreateResources(Renderer::ShaderHandle litShader);

        Batch CreateBatch(const Renderer::Mesh& mesh, uint32_t capacity);
        void GrowBatch(Batch&, uint32_t capacity);

        template <typename GpuLight>
        LightBuffer CreateLightBuffer(uint32_t capacity);

        // grow if needed -> write header + array -> bind
        template <typename GpuLight>
        void UploadLights(LightBuffer& buffer,
                          const std::vector<GpuLight>& lights,
                          uint32_t bindingPoint, Renderer::CommandList& cmd);

    private:
        std::unordered_map<BatchKey, Batch, BatchKeyHasher> batches_{};

        Renderer::GraphicsDevice& device_;
        Renderer::PipelineHandle pipeline_;
        Renderer::BufferHandle frameUBO_;
        Renderer::BufferHandle environmentUBO_;

        LightBuffer directionalLightSSBO_{};
        LightBuffer pointLightSSBO_{};
        LightBuffer spotLightSSBO_{};
        std::vector<Renderer::Layout::GpuDirectionalLight> directionalLights_;
        std::vector<Renderer::Layout::GpuPointLight> pointLights_;
        std::vector<Renderer::Layout::GpuSpotLight> spotLights_;

        bool instancing_ = true;

        Renderer::ShaderHandle litShader_{};

        Renderer::TextureHandle defaultAlbedo_;
        Renderer::TextureHandle defaultNormal_;
    };
} // namespace GEF::Scene

inline std::size_t GEF::Scene::SceneRenderer::BatchKeyHasher::operator()(
    const BatchKey& key) const noexcept
{
    return Utils::HashAll(key.mesh, key.albedoMap, key.normalMap);
}
