//
// Path: tests/Utils/FrameTimer.test.cpp
//
// Pure computation, no GPU. The frame times are powers of two (0.25, 0.125...)
// so their sums are exact in float: the one-second window closes exactly
// where the test expects, without rounding noise.
//

#include <doctest.h>

#include <Engine/Utils/FrameTimer.hpp>

using GEF::Utils::FrameTimer;

TEST_CASE("Nothing is reported before a full second has passed")
{
    FrameTimer timer;
    timer.Update(0.25f);
    timer.Update(0.25f);
    timer.Update(0.25f);

    CHECK(timer.GetFPS() == 0.0f);
    CHECK(timer.GetAvgFrameTime() == 0.0f);
    CHECK(timer.GetWorstFrameTime() == 0.0f);
}

TEST_CASE("A steady frame rate gives matching FPS, average and worst")
{
    FrameTimer timer;
    for (int i = 0; i < 8; ++i)
        timer.Update(0.125f); // 8 frames in exactly 1 second

    CHECK(timer.GetFPS() == doctest::Approx(8.0f));
    CHECK(timer.GetAvgFrameTime() == doctest::Approx(0.125f));
    CHECK(timer.GetWorstFrameTime() == doctest::Approx(0.125f));
}

TEST_CASE("The worst frame time reveals a stutter the average hides")
{
    FrameTimer timer;
    for (int i = 0; i < 4; ++i)
        timer.Update(0.125f);
    timer.Update(0.5f); // one long frame: 5 frames, 1 second in total

    CHECK(timer.GetFPS() == doctest::Approx(5.0f));
    CHECK(timer.GetAvgFrameTime() == doctest::Approx(0.2f));
    CHECK(timer.GetWorstFrameTime() == doctest::Approx(0.5f)); // the stutter
}

TEST_CASE("Each one-second window starts fresh")
{
    FrameTimer timer;

    // First window: a 0.5 s spike
    timer.Update(0.5f);
    timer.Update(0.5f);
    REQUIRE(timer.GetWorstFrameTime() == doctest::Approx(0.5f));

    // Second window: smooth frames only, the old spike must not linger
    for (int i = 0; i < 4; ++i)
        timer.Update(0.25f);

    CHECK(timer.GetFPS() == doctest::Approx(4.0f));
    CHECK(timer.GetWorstFrameTime() == doctest::Approx(0.25f));
}

TEST_CASE("The reported values stay stable until the next window closes")
{
    FrameTimer timer;
    for (int i = 0; i < 4; ++i)
        timer.Update(0.25f);
    REQUIRE(timer.GetFPS() == doctest::Approx(4.0f));

    // Mid-window: still showing the last complete second
    timer.Update(0.125f);
    CHECK(timer.GetFPS() == doctest::Approx(4.0f));
    CHECK(timer.GetAvgFrameTime() == doctest::Approx(0.25f));
}
