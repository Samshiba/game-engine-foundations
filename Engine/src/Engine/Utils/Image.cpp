//
// Created by genin on 01/11/2026.
// Path: Engine/src/Engine/Utils/Image.cpp
//

#include "Image.hpp"

#include <algorithm>

#include <Engine/Core/Log.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace GEF::Utils
{
    namespace
    {
        void FlipInplace(std::vector<uint8_t>& pixels, uint32_t width,
                         uint32_t height)
        {
            const uint32_t rowSize = width * 4;

            for (uint32_t y = 0; y < height / 2; ++y)
            {
                auto topRow = pixels.begin() + (y * rowSize);

                auto bottomRow = pixels.begin() + ((height - 1 - y) * rowSize);

                std::swap_ranges(topRow, topRow + rowSize, bottomRow);
            }
        }
    } // namespace

    void WritePng(const std::filesystem::path& path, uint32_t width,
                  uint32_t height, std::vector<uint8_t> pixels)
    {
        FlipInplace(pixels, width, height);
        int result = stbi_write_png(path.string().c_str(), width, height, 4,
                                    pixels.data(), width * 4);
        if (result == 0)
        {
            GEF_ENGINE_ERROR("Failed to write PNG file: {}", path.string());
        }
    }
} // namespace GEF::Utils