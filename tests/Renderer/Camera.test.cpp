//
// Path: tests/Renderer/Camera.test.cpp
//
// Pure math, no GPU: runs on every CI job.
//

#include <doctest.h>

#include <Engine/Renderer/Camera.hpp>

#include <glm/glm.hpp>

using GEF::Renderer::Camera;

namespace
{
    constexpr float Epsilon = 1e-4f;

    bool Near(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::all(glm::lessThan(glm::abs(a - b), glm::vec3(Epsilon)));
    }

    bool Near(const glm::mat4& a, const glm::mat4& b)
    {
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                if (glm::abs(a[column][row] - b[column][row]) > Epsilon)
                    return false;
        return true;
    }

    // World point -> normalized device coordinates (after the w divide)
    glm::vec3 ToNdc(const Camera& camera, const glm::vec3& world)
    {
        const glm::vec4 clip = camera.GetViewProjection() * glm::vec4(world, 1.0f);
        return glm::vec3(clip) / clip.w;
    }
}

TEST_CASE("The default camera looks down -Z, with +X on its right")
{
    const Camera camera;

    CHECK(Near(camera.GetForward(), { 0.0f, 0.0f, -1.0f }));
    CHECK(Near(camera.GetRight(), { 1.0f, 0.0f, 0.0f }));
}

TEST_CASE("Yaw turns the camera around the vertical axis")
{
    Camera camera;

    camera.yaw = 0.0f;
    CHECK(Near(camera.GetForward(), { 1.0f, 0.0f, 0.0f }));

    camera.yaw = 90.0f;
    CHECK(Near(camera.GetForward(), { 0.0f, 0.0f, 1.0f }));

    camera.yaw = 180.0f;
    CHECK(Near(camera.GetForward(), { -1.0f, 0.0f, 0.0f }));
}

TEST_CASE("Pitch tilts the camera up and down")
{
    Camera camera;

    camera.pitch = 89.0f;
    CHECK(camera.GetForward().y > 0.99f);

    camera.pitch = -89.0f;
    CHECK(camera.GetForward().y < -0.99f);
}

TEST_CASE("Forward and right are orthonormal, right stays horizontal")
{
    Camera camera;
    for (const float yaw : { -90.0f, 0.0f, 37.0f, 135.0f, 270.0f })
    {
        for (const float pitch : { -89.0f, -45.0f, 0.0f, 30.0f, 89.0f })
        {
            camera.yaw = yaw;
            camera.pitch = pitch;
            CAPTURE(yaw);
            CAPTURE(pitch);

            const glm::vec3 forward = camera.GetForward();
            const glm::vec3 right = camera.GetRight();
            CHECK(glm::length(forward) == doctest::Approx(1.0f).epsilon(Epsilon));
            CHECK(glm::length(right) == doctest::Approx(1.0f).epsilon(Epsilon));
            CHECK(glm::dot(forward, right) == doctest::Approx(0.0f).epsilon(Epsilon));
            CHECK(right.y == doctest::Approx(0.0f).epsilon(Epsilon)); // no roll
        }
    }
}

TEST_CASE("The view matrix puts the camera at the origin, looking down -Z")
{
    Camera camera;
    camera.position = { 3.0f, 2.0f, 5.0f };
    camera.yaw = 0.0f; // looking down +X in the world

    const glm::mat4 view = camera.GetView();

    // The camera itself goes to the origin of view space
    CHECK(Near(glm::vec3(view * glm::vec4(camera.position, 1.0f)), { 0, 0, 0 }));

    // A point 4 units in front of it ends up 4 units down -Z (OpenGL convention)
    const glm::vec3 inFront = camera.position + 4.0f * camera.GetForward();
    CHECK(Near(glm::vec3(view * glm::vec4(inFront, 1.0f)), { 0, 0, -4.0f }));

    // A point on its right ends up on +X
    const glm::vec3 onRight = camera.position + camera.GetRight();
    CHECK(glm::vec3(view * glm::vec4(onRight, 1.0f)).x > 0.0f);
}

TEST_CASE("The near and far planes map to the edges of the depth range")
{
    Camera camera; // at (0, 0, 3), looking down -Z
    camera.nearPlane = 0.5f;
    camera.farPlane = 100.0f;

    // OpenGL clip space: near -> z = -1, far -> z = +1
    const glm::vec3 onNear = camera.position + camera.nearPlane * camera.GetForward();
    const glm::vec3 onFar = camera.position + camera.farPlane * camera.GetForward();
    CHECK(ToNdc(camera, onNear).z == doctest::Approx(-1.0f).epsilon(Epsilon));
    CHECK(ToNdc(camera, onFar).z == doctest::Approx(1.0f).epsilon(Epsilon));

    // In between: inside the depth range, so visible
    const glm::vec3 middle = camera.position + 10.0f * camera.GetForward();
    CHECK(ToNdc(camera, middle).z > -1.0f);
    CHECK(ToNdc(camera, middle).z < 1.0f);
}

TEST_CASE("fovY sets the vertical opening, the aspect ratio the horizontal one")
{
    Camera camera;
    camera.position = { 0, 0, 0 };
    camera.fovY = 90.0f;
    camera.aspectRatio = 2.0f;

    // With a 90 degree fovY, a point at 45 degrees up sits on the top edge
    CHECK(ToNdc(camera, { 0.0f, 1.0f, -1.0f }).y ==
          doctest::Approx(1.0f).epsilon(Epsilon));

    // The image is twice as wide as tall: the right edge is twice as far out
    CHECK(ToNdc(camera, { 2.0f, 0.0f, -1.0f }).x ==
          doctest::Approx(1.0f).epsilon(Epsilon));

    // A smaller fovY zooms in: the same point leaves the screen
    camera.fovY = 45.0f;
    CHECK(ToNdc(camera, { 0.0f, 1.0f, -1.0f }).y > 1.0f);
}

TEST_CASE("GetViewProjection is projection * view, in that order")
{
    Camera camera;
    camera.position = { 1.0f, -2.0f, 7.0f };
    camera.yaw = 33.0f;
    camera.pitch = -12.0f;

    CHECK(Near(camera.GetViewProjection(),
               camera.GetProjection() * camera.GetView()));
    CHECK_FALSE(Near(camera.GetViewProjection(),
                     camera.GetView() * camera.GetProjection()));
}
