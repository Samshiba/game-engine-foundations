//
// Created by genin on 01/11/2026.
// Path: Engine/src/Engine/Assets/ImageLoader.cpp
//

#include <Engine/Assets/ImageLoader.hpp>

#include <algorithm>

#include <Engine/Core/Log.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace GEF::Assets
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

    std::optional<Renderer::ImageData>
    LoadImage(const std::filesystem::path& path)
    {
        int width, height, nrChannels;
        unsigned char* data =
            stbi_load(path.string().c_str(), &width, &height, &nrChannels, 4);

        if (!data)
        {
            GEF_ENGINE_ERROR("Failed to load image: {}", path.string());
            return std::nullopt;
        }

        std::vector<uint8_t> pixels;
        pixels.assign(data, data + (width * height * 4));
        FlipInplace(pixels, width, height);

        Renderer::ImageData imageData;
        imageData.width = static_cast<uint32_t>(width);
        imageData.height = static_cast<uint32_t>(height);
        imageData.pixels = std::move(pixels);

        stbi_image_free(data);

        return imageData;
    }

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

    void WritePng(const std::filesystem::path& path,
                  Renderer::ImageData& imageData)
    {
        FlipInplace(imageData.pixels, imageData.width, imageData.height);
        int result = stbi_write_png(
            path.string().c_str(), imageData.width, imageData.height, 4,
            imageData.pixels.data(), imageData.width * 4);

        if (result == 0)
        {
            GEF_ENGINE_ERROR("Failed to write PNG file: {}", path.string());
        }
    }
} // namespace GEF::Assets