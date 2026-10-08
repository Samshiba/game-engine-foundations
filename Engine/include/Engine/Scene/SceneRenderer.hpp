//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Scene/SceneRenderer.hpp
//

#pragma once

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/Camera.hpp>

#include <libecs/core/registry/Registry.hpp>


namespace GEF::Scene
{
    class SceneRenderer
    {
    public:
        SceneRenderer(Renderer::GraphicsDevice& device,
                      Renderer::ShaderHandle litShader);
        ~SceneRenderer();

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;

        void Render(const libecs::core::registry::Registry& registry,
                    const Renderer::Camera& camera,
                    Renderer::CommandList& cmd);

    private:
        Renderer::GraphicsDevice& device_;
        Renderer::PipelineHandle pipeline_;
        Renderer::BufferHandle frameUBO_;
        Renderer::BufferHandle objectUBO_;
        Renderer::BufferHandle lightUBO_;
        Renderer::BufferHandle materialUBO_;
    };
}