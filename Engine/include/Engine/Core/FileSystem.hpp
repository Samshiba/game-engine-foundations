//
// Created by genin on 05/10/2026.
// Path: Engine/include/Engine/Core/FileSystem.hpp
//

#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace GEF::FileSystem
{
    [[nodiscard]] const std::filesystem::path& GetExecutableDirectory();
    [[nodiscard]] std::filesystem::path AssetsDirectory();
    [[nodiscard]] std::filesystem::path AssetPath(
        const std::filesystem::path& relative);
    [[nodiscard]] std::optional<std::string> ReadTextFile(
        const std::filesystem::path& path);
}