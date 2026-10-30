//
// Path: tests/Utils/FrameStatistics.test.cpp
//

#include <Engine/Utils/FrameStatistics.hpp>

#include <vector>

#include <doctest.h>

using namespace GEF::Utils;

TEST_CASE("No frame, no statistics")
{
    const FrameStatistics stats = ComputeFrameStatistics({});

    CHECK(stats.frames == 0);
    CHECK(stats.meanMs == 0.0f);
    CHECK(stats.fps == 0.0f);
}

TEST_CASE("A steady series: every statistic is the same value")
{
    const FrameStatistics stats =
        ComputeFrameStatistics(std::vector<float>(100, 4.0f));

    CHECK(stats.frames == 100);
    CHECK(stats.meanMs == doctest::Approx(4.0f));
    CHECK(stats.p50Ms == doctest::Approx(4.0f));
    CHECK(stats.p99Ms == doctest::Approx(4.0f));
    CHECK(stats.worstMs == doctest::Approx(4.0f));
    CHECK(stats.fps == doctest::Approx(250.0f));
}

TEST_CASE("Percentiles use the nearest rank of the sorted series")
{
    // 1, 2, ..., 100 in reverse order: the input order must not matter
    std::vector<float> times;
    for (int i = 100; i >= 1; --i)
        times.push_back(float(i));

    const FrameStatistics stats = ComputeFrameStatistics(times);

    CHECK(stats.p50Ms == doctest::Approx(50.0f));
    CHECK(stats.p95Ms == doctest::Approx(95.0f));
    CHECK(stats.p99Ms == doctest::Approx(99.0f));
    CHECK(stats.worstMs == doctest::Approx(100.0f));
    CHECK(stats.meanMs == doctest::Approx(50.5f));
}

TEST_CASE("p99 exposes a stutter the mean hides")
{
    // 98 smooth frames at 5 ms, 2 hitches at 50 ms
    std::vector<float> times(98, 5.0f);
    times.push_back(50.0f);
    times.push_back(50.0f);

    const FrameStatistics stats = ComputeFrameStatistics(times);

    CHECK(stats.meanMs == doctest::Approx(5.9f)); // looks fine
    CHECK(stats.p50Ms == doctest::Approx(5.0f));
    CHECK(stats.p99Ms == doctest::Approx(50.0f)); // the hitches show up
}

TEST_CASE("Percentile of a tiny series stays inside it")
{
    const std::vector<float> one{ 7.0f };
    CHECK(Percentile(one, 0.0f) == 7.0f);
    CHECK(Percentile(one, 50.0f) == 7.0f);
    CHECK(Percentile(one, 100.0f) == 7.0f);
    CHECK(Percentile({}, 50.0f) == 0.0f);
}
