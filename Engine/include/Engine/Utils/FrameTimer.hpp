//
// Created by genin on 08/10/2026.
// Path: Engine/include/Engine/Utils/FrameTimer.hpp
//

#pragma once

namespace GEF::Utils
{
    class FrameTimer
    {
    public:
        void Update(float dt)
        {
            accumulatedTime_ += dt;
            frameCount_++;
            if (dt > currentWorst_)
                currentWorst_ = dt;

            if (accumulatedTime_ >= 1.0f)
            {
                fps_ = static_cast<float>(frameCount_) / accumulatedTime_;
                avgFrameTime_ = accumulatedTime_ / frameCount_;
                worstFrameTime_ = currentWorst_;

                accumulatedTime_ = 0.0f;
                frameCount_ = 0;
                currentWorst_ = 0.0f;
            }
        }

        float GetFPS() const
        {
            return fps_;
        }

        float GetAvgFrameTime() const
        {
            return avgFrameTime_;
        }

        float GetWorstFrameTime() const
        {
            return worstFrameTime_;
        }

    private:
        float accumulatedTime_ = 0.0f;
        int frameCount_ = 0;
        float currentWorst_ = 0.0f;
        float fps_ = 0.0f;
        float avgFrameTime_ = 0.0f;
        float worstFrameTime_ = 0.0f;
    };
}