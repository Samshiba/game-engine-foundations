//
// Created by genin on 06/10/2026.
// Path: Engine/include/Engine/Assets/ObjLoader.hpp
//

#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

#include <Engine/Renderer/Mesh.hpp>

namespace GEF::Assets
{
    [[nodiscard]] std::optional<Renderer::MeshData>
    ParseObj(std::string_view objText);

    [[nodiscard]] std::optional<Renderer::MeshData>
    LoadObj(const std::filesystem::path& path);
} // namespace GEF::Assets
