//
// Created by genin on 19/11/2026.
// Path: Application/src/Model.hpp
//

#pragma once

#include <string_view>

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/Mesh.hpp>
#include <Engine/Renderer/Texture.hpp>

// A mesh and its two maps, loaded from assets/models and assets/textures
struct Model
{
    GEF::Renderer::Mesh mesh{};
    GEF::Renderer::Texture albedoMap{};
    GEF::Renderer::Texture normalMap{};
};

Model LoadModel(GEF::Renderer::GraphicsDevice& device, std::string_view obj,
                std::string_view albedoMap, std::string_view normalMap);
void DestroyModel(GEF::Renderer::GraphicsDevice& device, const Model& model);
