//
// Created by genin on 19/11/2026.
// Path: Application/src/Model.cpp
//

#include "Model.hpp"

#include <Engine/Assets/ImageLoader.hpp>
#include <Engine/Assets/ObjLoader.hpp>
#include <Engine/Core/FileSystem.hpp>
#include <Engine/Core/Log.hpp>

namespace
{
    GEF::Renderer::Texture LoadTexture(GEF::Renderer::GraphicsDevice& device,
                                       std::string_view name,
                                       GEF::Renderer::TextureFormat format,
                                       GEF::Renderer::TextureWrap wrap)
    {
        const auto path =
            GEF::FileSystem::AssetPath(std::string("textures/") += name);
        if (const auto image = GEF::Assets::LoadImage(path))
            return UploadTexture(device, *image, format, wrap);

        GEF_ERROR("Failed to load texture: {}", path.string());
        return {};
    }
} // namespace

Model LoadModel(GEF::Renderer::GraphicsDevice& device, std::string_view obj,
                std::string_view albedoMap, std::string_view normalMap)
{
    Model model;

    const auto path = GEF::FileSystem::AssetPath(std::string("models/") += obj);
    if (const auto data = GEF::Assets::LoadObj(path))
        model.mesh = UploadMesh(device, *data);
    else
        GEF_ERROR("Failed to load mesh: {}", path.string());

    // Albedo is authored in sRGB, the normal map holds plain vectors
    model.albedoMap =
        LoadTexture(device, albedoMap, GEF::Renderer::TextureFormat::SRGBA8,
                    GEF::Renderer::TextureWrap::Repeat);
    model.normalMap =
        LoadTexture(device, normalMap, GEF::Renderer::TextureFormat::RGBA8,
                    GEF::Renderer::TextureWrap::Repeat);
    return model;
}

void DestroyModel(GEF::Renderer::GraphicsDevice& device, const Model& model)
{
    if (model.mesh.indexCount != 0)
        DestroyMesh(device, model.mesh);
    DestroyTexture(device, model.albedoMap);
    DestroyTexture(device, model.normalMap);
}
