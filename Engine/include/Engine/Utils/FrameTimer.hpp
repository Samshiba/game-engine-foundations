//
// Created by genin on 08/10/2026.
// Path: Engine/include/Engine/Utils/FrameTimer.hpp
//

#pragma once

#include "FrameStatistics.hpp"

#include <vector>

namespace GEF::Utils
{
    class FrameTimer
    {
    public:
        void Update(float dt)
        {
            accumulatedTime_ += dt;
            frameTimesMs_.push_back(dt * 1000.0f);

            if (accumulatedTime_ >= 1.0f)
            {
                stats_ = ComputeFrameStatistics(frameTimesMs_);

                accumulatedTime_ = 0.0f;
                frameTimesMs_.clear();
            }
        }

        // Mean, percentiles and worst of the last complete second, in ms
        const FrameStatistics& GetStatistics() const
        {
            return stats_;
        }

        float GetFPS() const
        {
            return stats_.fps;
        }

        // In seconds
        float GetAvgFrameTime() const
        {
            return stats_.meanMs / 1000.0f;
        }

        // In seconds
        float GetWorstFrameTime() const
        {
            return stats_.worstMs / 1000.0f;
        }

    private:
        float accumulatedTime_ = 0.0f;
        std::vector<float> frameTimesMs_;
        FrameStatistics stats_;
    };
}
