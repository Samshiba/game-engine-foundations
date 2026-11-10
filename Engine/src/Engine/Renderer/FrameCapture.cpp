//
// Created by genin on 01/11/2026.
// Path: Engine/src/Engine/Renderer/FrameCapture.cpp
//

#include <Engine/Renderer/FrameCapture.hpp>

#include <chrono>
#include <format>

#include <Engine/Assets/ImageLoader.hpp>
#include <Engine/Core/FileSystem.hpp>
#include <Engine/Core/Log.hpp>

namespace GEF::Renderer
{
    namespace
    {
        std::filesystem::path Timestamp()
        {
            return std::format("{:%Y%m%d-%H%M%S}",
                               std::chrono::floor<std::chrono::seconds>(
                                   std::chrono::system_clock::now()));
        }
    } // namespace

    void FrameCapture::RequestScreenshot()
    {
        if (isCapturing_)
            return;

        GEF_ENGINE_DEBUG("Requesting screenshot");
        isCapturing_ = true;
        frameCount_ = 1;
        isSequence_ = false;
        output_ = FileSystem::GetExecutableDirectory() / "screenshots"
            / std::format("{}.png", Timestamp().string());
        std::filesystem::create_directories(output_.parent_path());
    }
    void FrameCapture::RequestSequence(uint32_t frameCount)
    {
        if (isCapturing_)
            return;

        GEF_ENGINE_DEBUG("Requesting sequence");
        isCapturing_ = true;
        frameCount_ = frameCount;
        isSequence_ = true;
        output_ =
            FileSystem::GetExecutableDirectory() / "screenshots" / Timestamp();
        std::filesystem::create_directories(output_);
    }

    bool FrameCapture::IsCapturing() const
    {
        return isCapturing_;
    }

    void FrameCapture::Export(uint32_t width, uint32_t height)
    {
        for (uint32_t i = 0; i < capturedFrames_.size(); ++i)
        {
            const auto path = isSequence_
                ? output_ / std::format("frame_{:04}.png", i)
                : output_;
            Assets::WritePng(path, width, height,
                             std::move(capturedFrames_[i]));
        }
        capturedFrames_.clear();
    }

    void FrameCapture::OnEndFrame(GraphicsDevice& device, uint32_t width,
                                  uint32_t height)
    {
        if (!isCapturing_)
            return;

        GEF_ENGINE_INFO("Capturing frame");
        capturedFrames_.push_back(device.ReadFramebuffer(width, height));

        frameCount_--;
        if (frameCount_ == 0)
        {
            isCapturing_ = false;
            GEF_ENGINE_INFO("Finished capturing, exporting");
            Export(width, height);
        }
    }
} // namespace GEF::Renderer