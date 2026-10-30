//
// Path: tests/Scene/Transform.test.cpp
//
// Pure math, no GPU: runs on every CI job.
//

#include <doctest.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Engine/Scene/Components.hpp>

using GEF::Scene::Transform;

namespace
{
    constexpr float Epsilon = 1e-4f;

    bool Near(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::all(glm::lessThan(glm::abs(a - b), glm::vec3(Epsilon)));
    }

    glm::vec3 Apply(const Transform& transform, const glm::vec3& point)
    {
        return glm::vec3(transform.GetMatrix() * glm::vec4(point, 1.0f));
    }
} // namespace

TEST_CASE("A default Transform is the identity")
{
    const Transform transform;

    CHECK(transform.GetMatrix() == glm::mat4(1.0f));
    CHECK(Near(Apply(transform, { 1, 2, 3 }), { 1, 2, 3 }));
}

TEST_CASE("Position translates, rotation turns, scale stretches")
{
    SUBCASE("position")
    {
        const Transform transform{ .position = { 5, -1, 2 } };
        CHECK(Near(Apply(transform, { 0, 0, 0 }), { 5, -1, 2 }));
    }

    SUBCASE("rotation: a quaternion from an angle and an axis")
    {
        // +90 degrees around Y turns +X into -Z (right-handed)
        const Transform transform{
            .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0))
        };
        CHECK(Near(Apply(transform, { 1, 0, 0 }), { 0, 0, -1 }));
    }

    SUBCASE("scale")
    {
        const Transform transform{ .scale = { 2, 3, 4 } };
        CHECK(Near(Apply(transform, { 1, 1, 1 }), { 2, 3, 4 }));
    }
}

TEST_CASE(
    "GetMatrix applies scale, then rotation, then translation (T * R * S)")
{
    const Transform transform{ .position = { 10, 0, 0 },
                               .rotation = glm::angleAxis(glm::radians(90.0f),
                                                          glm::vec3(0, 1, 0)),
                               .scale = glm::vec3(2.0f) };

    // (1,0,0) -> scale -> (2,0,0) -> rotate -> (0,0,-2) -> translate ->
    // (10,0,-2)
    CHECK(Near(Apply(transform, { 1, 0, 0 }), { 10, 0, -2 }));

    // The object's origin only moves by the translation: scaling and rotating
    // happen around the object itself, not around the world origin
    CHECK(Near(Apply(transform, { 0, 0, 0 }), { 10, 0, 0 }));
}

TEST_CASE("GetNormalMatrix is R * S^-1, the inverse transpose of a T * R * S")
{
    // Instancing sends one normal matrix per instance: R * S^-1 avoids a
    // general 3x3 inversion per entity per frame, and is exactly equal to it
    const Transform transform{
        .position = { 3, -2, 7 },
        .rotation = glm::angleAxis(glm::radians(40.0f),
                                   glm::normalize(glm::vec3(1, 2, 3))),
        .scale = { 2.0f, 0.5f, 3.0f }
    }; // non-uniform: where it matters

    const glm::mat3 expected =
        glm::transpose(glm::inverse(glm::mat3(transform.GetMatrix())));
    const glm::mat3 normalMatrix = transform.GetNormalMatrix();

    for (int column = 0; column < 3; ++column)
        CHECK(Near(normalMatrix[column], expected[column]));
}

TEST_CASE("A normal stays perpendicular to its surface under non-uniform scale")
{
    // A 45 degree slope in XY, stretched 4x along X
    const Transform transform{ .scale = { 4.0f, 1.0f, 1.0f } };
    const glm::vec3 tangent = glm::normalize(glm::vec3(1, 1, 0));
    const glm::vec3 normal = glm::normalize(glm::vec3(-1, 1, 0));

    const glm::vec3 scaledTangent = glm::mat3(transform.GetMatrix()) * tangent;

    // mat3(model) tilts the normal: no longer perpendicular (the classic bug)
    const glm::vec3 wrong = glm::mat3(transform.GetMatrix()) * normal;
    CHECK(glm::abs(glm::dot(wrong, scaledTangent)) > 0.1f);

    // The normal matrix keeps it perpendicular
    const glm::vec3 right = transform.GetNormalMatrix() * normal;
    CHECK(glm::dot(right, scaledTangent)
          == doctest::Approx(0.0f).epsilon(1e-4));
}
