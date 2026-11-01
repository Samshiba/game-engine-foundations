//
// Created by genin on 01/11/2026.
// Path: Engine/include/Engine/Renderer/FrameCapture.hpp
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <Engine/Renderer/GraphicsDevice.hpp>

namespace GEF::Renderer
{
    class FrameCapture
    {
    public:
        static constexpr uint32_t SequenceFrameRate = 30;

    public:
        void RequestScreenshot();
        void RequestSequence(uint32_t frameCount);
        [[nodiscard]] bool IsCapturing() const;

        void OnEndFrame(GraphicsDevice& device, uint32_t width,
                        uint32_t height);

    private:
        void Export(uint32_t width, uint32_t height);

    private:
        std::vector<std::vector<uint8_t>> capturedFrames_;

        bool isCapturing_ = false;
        uint32_t frameCount_ = 0;
        bool isSequence_ = false;
        std::filesystem::path output_;
    };
} // namespace GEF::Renderer