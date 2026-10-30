//
// Created by genin on 22/10/2026.
// Path: Engine/include/Engine/Utils/FrameStatistics.hpp
//

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <vector>

namespace GEF::Utils
{
    struct FrameStatistics
    {
        std::size_t frames = 0;
        float meanMs = 0.0f;
        float p50Ms = 0.0f; // median
        float p95Ms = 0.0f;
        float p99Ms = 0.0f;
        float worstMs = 0.0f;
        float fps = 0.0f; // 1000 / mean: frames per second over the run
    };

    // Nearest-rank percentile of a sorted series: the smallest value with at
    // least `percent` % of the samples at or below it
    [[nodiscard]] inline float Percentile(const std::vector<float>& sorted,
                                          float percent)
    {
        if (sorted.empty())
            return 0.0f;

        const auto rank = static_cast<std::size_t>(
            std::ceil(percent / 100.0f * static_cast<float>(sorted.size())));

        return sorted[std::clamp<std::size_t>(rank, 1, sorted.size()) - 1];
    }

    [[nodiscard]] inline FrameStatistics
    ComputeFrameStatistics(std::vector<float> frameTimesMs)
    {
        FrameStatistics stats;
        if (frameTimesMs.empty())
            return stats;

        std::sort(frameTimesMs.begin(), frameTimesMs.end());

        stats.frames = frameTimesMs.size();
        // Accumulated in double: summing thousands of floats drifts
        const double sum =
            std::accumulate(frameTimesMs.begin(), frameTimesMs.end(), 0.0);
        stats.meanMs =
            static_cast<float>(sum / static_cast<double>(stats.frames));
        stats.p50Ms = Percentile(frameTimesMs, 50.0f);
        stats.p95Ms = Percentile(frameTimesMs, 95.0f);
        stats.p99Ms = Percentile(frameTimesMs, 99.0f);
        stats.worstMs = frameTimesMs.back();
        stats.fps = stats.meanMs > 0.0f ? 1000.0f / stats.meanMs : 0.0f;
        return stats;
    }
} // namespace GEF::Utils
