//
// Created by genin on 01/11/2026.
// Path: Engine/src/Engine/Utils/Image.hpp
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace GEF::Utils
{
    void WritePng(const std::filesystem::path& path, uint32_t width,
                  uint32_t height, std::vector<uint8_t> pixels);
} // namespace GEF::Utils