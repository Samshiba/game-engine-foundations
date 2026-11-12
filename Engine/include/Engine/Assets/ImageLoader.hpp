//
// Created by genin on 10/11/2026.
// Path: Engine/include/Engine/Assets/ImageLoader.hpp
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/Texture.hpp>

namespace GEF::Assets
{
    [[nodiscard]] std::optional<Renderer::ImageData>
    LoadImage(const std::filesystem::path& path);

    void WritePng(const std::filesystem::path& path, uint32_t width,
                  uint32_t height, std::vector<uint8_t> pixels);
    void WritePng(const std::filesystem::path& path,
                  Renderer::ImageData& imageData);
} // namespace GEF::Assets