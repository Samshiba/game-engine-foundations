//
// Path: tests/Assets/ObjLoader.test.cpp
//
// CPU side of the mesh loading (ParseObj / LoadObj): no GPU, runs on every
// CI job. The OBJ files are written inline, small enough to check by hand.
//

#include <doctest.h>

#include <Engine/Assets/ObjLoader.hpp>

#include <glm/glm.hpp>

#include <filesystem>
#include <fstream>
#include <string>

using namespace GEF::Renderer;
using GEF::Assets::LoadObj;
using GEF::Assets::ParseObj;
namespace fs = std::filesystem;

namespace
{
    constexpr float Epsilon = 1e-4f;

    bool Near(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::all(glm::lessThan(glm::abs(a - b), glm::vec3(Epsilon)));
    }

    // One triangle in the XY plane, counter-clockwise seen from +Z
    constexpr std::string_view Triangle = R"(
v 0 0 0
v 1 0 0
v 0 1 0
f 1 2 3
)";

    // A unit cube centered on the origin, 8 shared positions, no normals
    constexpr std::string_view CubeWithoutNormals = R"(
v -0.5 -0.5  0.5
v  0.5 -0.5  0.5
v  0.5  0.5  0.5
v -0.5  0.5  0.5
v -0.5 -0.5 -0.5
v  0.5 -0.5 -0.5
v  0.5  0.5 -0.5
v -0.5  0.5 -0.5
f 1 2 3 4
f 6 5 8 7
f 2 6 7 3
f 5 1 4 8
f 4 3 7 8
f 5 6 2 1
)";

    // The same cube with one normal per face: a corner is shared by 3 faces
    // with 3 different normals, so it must become 3 different vertices
    constexpr std::string_view CubeWithFaceNormals = R"(
v -0.5 -0.5  0.5
v  0.5 -0.5  0.5
v  0.5  0.5  0.5
v -0.5  0.5  0.5
v -0.5 -0.5 -0.5
v  0.5 -0.5 -0.5
v  0.5  0.5 -0.5
v -0.5  0.5 -0.5
vn  0  0  1
vn  0  0 -1
vn  1  0  0
vn -1  0  0
vn  0  1  0
vn  0 -1  0
f 1//1 2//1 3//1 4//1
f 6//2 5//2 8//2 7//2
f 2//3 6//3 7//3 3//3
f 5//4 1//4 4//4 8//4
f 4//5 3//5 7//5 8//5
f 5//6 6//6 2//6 1//6
)";
}

TEST_CASE("ParseObj reads a triangle")
{
    const auto mesh = ParseObj(Triangle);

    REQUIRE(mesh.has_value());
    CHECK(mesh->vertices.size() == 3);
    REQUIRE(mesh->indices.size() == 3);
    CHECK(Near(mesh->vertices[mesh->indices[1]].position, { 1, 0, 0 }));
}

TEST_CASE("ParseObj triangulates polygons")
{
    // One quad face -> two triangles, still 4 distinct vertices
    const auto mesh = ParseObj(R"(
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
f 1 2 3 4
)");

    REQUIRE(mesh.has_value());
    CHECK(mesh->vertices.size() == 4);
    CHECK(mesh->indices.size() == 6);
}

TEST_CASE("ParseObj shares identical vertices between faces")
{
    // Two triangles sharing an edge: 6 corners, but only 4 distinct vertices
    const auto mesh = ParseObj(R"(
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
f 1 2 3
f 1 3 4
)");

    REQUIRE(mesh.has_value());
    CHECK(mesh->vertices.size() == 4);
    REQUIRE(mesh->indices.size() == 6);
    CHECK(mesh->indices[0] == mesh->indices[3]); // corner 1, reused
    CHECK(mesh->indices[2] == mesh->indices[4]); // corner 3, reused
}

TEST_CASE("Every index points to an existing vertex")
{
    const auto mesh = ParseObj(CubeWithFaceNormals);

    REQUIRE(mesh.has_value());
    for (const uint32_t index : mesh->indices)
        CHECK(index < mesh->vertices.size());
}

TEST_CASE("ParseObj keeps the normals of the file")
{
    const auto mesh = ParseObj(CubeWithFaceNormals);

    REQUIRE(mesh.has_value());
    // 6 faces x 4 corners: same positions, different normals -> not merged
    CHECK(mesh->vertices.size() == 24);
    CHECK(mesh->indices.size() == 36);

    // Each face-normal points away from the cube center, along one axis
    for (const Vertex& vertex : mesh->vertices)
    {
        CHECK(glm::length(vertex.normal) == doctest::Approx(1.0f));
        CHECK(glm::dot(vertex.normal, vertex.position) > 0.0f);
    }
}

TEST_CASE("ParseObj computes normals when the file has none")
{
    SUBCASE("a flat triangle gets its face normal")
    {
        const auto mesh = ParseObj(Triangle);

        REQUIRE(mesh.has_value());
        for (const Vertex& vertex : mesh->vertices)
            CHECK(Near(vertex.normal, { 0, 0, 1 })); // CCW seen from +Z
    }

    SUBCASE("a closed mesh gets smooth, unit, outward normals")
    {
        const auto mesh = ParseObj(CubeWithoutNormals);

        REQUIRE(mesh.has_value());
        // No normals in the file: corners are shared, normals are averaged
        CHECK(mesh->vertices.size() == 8);
        CHECK(mesh->indices.size() == 36);

        for (const Vertex& vertex : mesh->vertices)
        {
            CAPTURE(vertex.position.x);
            CAPTURE(vertex.position.y);
            CAPTURE(vertex.position.z);
            CHECK(glm::length(vertex.normal) == doctest::Approx(1.0f));
            // Outward, towards the corner's own octant: every component has
            // the sign of the position. Not exactly the diagonal: normals are
            // area-weighted and each quad is split in 2 triangles, so a corner
            // gets a face's normal once or twice depending on the split.
            CHECK(glm::dot(vertex.normal, vertex.position) > 0.0f);
            for (int axis = 0; axis < 3; ++axis)
                CHECK(vertex.normal[axis] * vertex.position[axis] > 0.0f);
        }
    }
}

TEST_CASE("ParseObj reads UVs, and defaults them to zero when absent")
{
    SUBCASE("with UVs")
    {
        const auto mesh = ParseObj(R"(
v 0 0 0
v 1 0 0
v 0 1 0
vt 0.0 0.0
vt 1.0 0.0
vt 0.0 1.0
f 1/1 2/2 3/3
)");

        REQUIRE(mesh.has_value());
        const Vertex& second = mesh->vertices[mesh->indices[1]];
        CHECK(second.uv.x == doctest::Approx(1.0f));
        CHECK(second.uv.y == doctest::Approx(0.0f));
    }

    SUBCASE("without UVs (texcoord_index is -1: must not be read)")
    {
        const auto mesh = ParseObj(Triangle);

        REQUIRE(mesh.has_value());
        for (const Vertex& vertex : mesh->vertices)
        {
            CHECK(vertex.uv.x == 0.0f);
            CHECK(vertex.uv.y == 0.0f);
        }
    }
}

TEST_CASE("ParseObj rejects invalid files without crashing")
{
    SUBCASE("zero index (OBJ indices start at 1): tinyobj reports an error")
    {
        CHECK_FALSE(ParseObj("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 0 1 2\n").has_value());
    }

    SUBCASE("out of bounds index: tinyobj only warns, the loader must check")
    {
        // Reading position 9 of 3 would go past the end of the array
        CHECK_FALSE(ParseObj("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 9\n").has_value());
    }

    SUBCASE("nothing to draw")
    {
        CHECK_FALSE(ParseObj("").has_value());
        CHECK_FALSE(ParseObj("v 0 0 0\nv 1 0 0\n").has_value()); // no face
        CHECK_FALSE(ParseObj("this is not an obj file\n").has_value());
    }
}

TEST_CASE("LoadObj reads a file from any path")
{
    const fs::path path = fs::temp_directory_path() / "gef_mesh_test.obj";
    std::ofstream(path, std::ios::binary) << Triangle;

    const auto mesh = LoadObj(path);

    REQUIRE(mesh.has_value());
    CHECK(mesh->indices.size() == 3);

    std::error_code ignored;
    fs::remove(path, ignored);
}

TEST_CASE("LoadObj returns nullopt for a missing file")
{
    const fs::path missing =
        fs::temp_directory_path() / "gef_this_mesh_does_not_exist.obj";

    CHECK_FALSE(LoadObj(missing).has_value());
}

TEST_CASE("Vertex matches the 32-byte layout UploadMesh describes")
{
    // position (12) + normal (12) + uv (8), no padding
    CHECK(sizeof(Vertex) == 32);
    CHECK(offsetof(Vertex, normal) == 12);
    CHECK(offsetof(Vertex, uv) == 24);
}
