//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Assets/ObjLoader.hpp
//

#pragma once

#include <Engine/Renderer/Mesh.hpp>

#include <filesystem>
#include <optional>
#include <string_view>

namespace GEF::Assets
{
    [[nodiscard]] std::optional<Renderer::MeshData> ParseObj(
        std::string_view objText);

    [[nodiscard]] std::optional<Renderer::MeshData> LoadObj(
        const std::filesystem::path& path);
}
