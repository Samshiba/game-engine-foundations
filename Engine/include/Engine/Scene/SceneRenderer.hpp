//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Scene/SceneRenderer.hpp
//

#pragma once

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/Camera.hpp>
#include <Engine/Renderer/Mesh.hpp>

#include <libecs/core/registry/Registry.hpp>


namespace GEF::Scene
{
    class SceneRenderer
    {
    private:
        struct Batch
        {
            Renderer::BufferHandle instanceBuffer;
            Renderer::VertexArrayHandle vertexArray;
            uint32_t capacity = 0; // instances the buffer can hold
            std::vector<Renderer::InstanceData> instances; // filled each frame
            Renderer::Mesh mesh;
        };

        static constexpr uint32_t InitialBatchCapacity = 256;

        using MeshKey = Renderer::VertexArrayHandle;

    public:
        SceneRenderer(Renderer::GraphicsDevice& device,
                      Renderer::ShaderHandle litShader);
        ~SceneRenderer();

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;

        void Render(const libecs::core::registry::Registry& registry,
                    const Renderer::Camera& camera,
                    Renderer::CommandList& cmd);

        void SetInstancingEnabled(bool enabled);
        [[nodiscard]] bool IsInstancingEnabled() const;
        [[nodiscard]] uint32_t GetDrawCallCount() const;

    private:
        Batch CreateBatch(const Renderer::Mesh& mesh, uint32_t capacity);
        void GrowBatch(Batch&, uint32_t capacity);

    private:
        std::unordered_map<MeshKey, Batch> batches_{};

        Renderer::GraphicsDevice& device_;
        Renderer::PipelineHandle pipeline_;
        Renderer::BufferHandle frameUBO_;
        Renderer::BufferHandle lightUBO_;

        bool instancing_ = true;
        uint32_t drawCalls_ = 0;
    };
}