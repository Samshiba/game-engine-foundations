//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Scene/SceneRenderer.hpp
//

#pragma once

#include <libecs/core/registry/Registry.hpp>

#include <Engine/Renderer/Camera.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/Layout/LitLayout.hpp>
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

        enum class DebugView : uint32_t
        {
            None,
            Albedo,
            Normal,
            UV,
            Tangent,
            NormalMap,
            LightingOnly,
        };

        struct ShadowSettings
        {
            uint32_t resolution = 2048;
            float bias = 0.005f;
            float normalBias = 0.05f;
            float texelSize = 1.0f;
            float distance = 50.0f;
        };

    private:
        struct Batch
        {
            Renderer::BufferHandle instanceBuffer;
            Renderer::VertexArrayHandle vertexArray;
            uint32_t capacity = 0; // instances the buffer can hold
            std::vector<Renderer::Layout::ShadingInstance>
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
                      ShadowSettings shadowSettings,
                      Renderer::ShaderHandle litShader);
        explicit SceneRenderer(Renderer::GraphicsDevice& device,
                               ShadowSettings shadowSettings);
        ~SceneRenderer();

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;

        void Prepare(const libecs::core::registry::Registry& registry,
                     const Renderer::Camera& camera,
                     const Environment& environment);

        void Render(Renderer::CommandList& cmd);
        void RenderShadow(Renderer::CommandList& cmd);

        void SetInstancingEnabled(bool enabled);
        [[nodiscard]] bool IsInstancingEnabled() const;

        void SetDebugView(DebugView debugView);
        [[nodiscard]] DebugView GetDebugView() const;

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
        bool instancing_ = true;

        // Lit shader
        Renderer::ShaderHandle litShader_{};
        Renderer::PipelineHandle litPipeline_;

        Renderer::BufferHandle litViewUBO_;
        Renderer::BufferHandle environmentUBO_;
        Renderer::BufferHandle shadowUBO_;

        LightBuffer directionalLightSSBO_{};
        LightBuffer pointLightSSBO_{};
        LightBuffer spotLightSSBO_{};
        std::vector<Renderer::Layout::GpuDirectionalLight> directionalLights_;
        std::vector<Renderer::Layout::GpuPointLight> pointLights_;
        std::vector<Renderer::Layout::GpuSpotLight> spotLights_;

        DebugView debugView_ = DebugView::None;

        Renderer::TextureHandle defaultAlbedo_;
        Renderer::TextureHandle defaultNormal_;

        ShadowSettings shadowSettings_;

        // Shadow shader
        Renderer::ShaderHandle shadowShader_{};
        Renderer::PipelineHandle shadowPipeline_;
        Renderer::RenderTargetHandle shadowTarget_;

        Renderer::BufferHandle shadowViewUBO_;
    };
} // namespace GEF::Scene

inline std::size_t GEF::Scene::SceneRenderer::BatchKeyHasher::operator()(
    const BatchKey& key) const noexcept
{
    return Utils::HashAll(key.mesh, key.albedoMap, key.normalMap);
}
