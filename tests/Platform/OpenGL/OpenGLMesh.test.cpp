//
// Path: tests/Platform/OpenGL/OpenGLMesh.test.cpp
//
// GPU side of the meshes: UploadMesh / DestroyMesh on the OpenGL backend.
//

#include <doctest.h>

#include "GLTestContext.hpp"

#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/Mesh.hpp>

#include <vector>

using namespace GEF;
using Renderer::MeshData;
using Renderer::Vertex;

namespace
{
    MeshData Quad()
    {
        MeshData data;
        data.vertices = {
            { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0 } },
            { { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0 } },
            { { 1, 1, 0 }, { 0, 0, 1 }, { 1, 1 } },
            { { 0, 1, 0 }, { 0, 0, 1 }, { 0, 1 } },
        };
        data.indices = { 0, 1, 2, 0, 2, 3 };
        return data;
    }
}

GL_TEST_CASE("UploadMesh creates buffers holding all the vertices and indices")
{
    Platform::OpenGLGraphicsDevice device;
    const MeshData data = Quad();

    const Renderer::Mesh mesh = Renderer::UploadMesh(device, data);

    CHECK(mesh.indexCount == 6);
    REQUIRE(device.GetVertexArray(mesh.vertexArray) != nullptr);

    // The whole arrays, not sizeof(pointer)
    const Platform::GLBuffer* vertices = device.GetBuffer(mesh.vertexBuffer);
    const Platform::GLBuffer* indices = device.GetBuffer(mesh.indexBuffer);
    REQUIRE(vertices != nullptr);
    REQUIRE(indices != nullptr);
    CHECK(vertices->type == Renderer::BufferType::Vertex);
    CHECK(vertices->size == 4 * sizeof(Vertex));
    CHECK(indices->type == Renderer::BufferType::Index);
    CHECK(indices->size == 6 * sizeof(uint32_t));

    // The data really reached the GPU
    std::vector<Vertex> readBack(4);
    glGetNamedBufferSubData(vertices->id, 0, 4 * sizeof(Vertex), readBack.data());
    CHECK(readBack[2].position.x == doctest::Approx(1.0f));
    CHECK(readBack[2].uv.y == doctest::Approx(1.0f));

    Renderer::DestroyMesh(device, mesh);
}

GL_TEST_CASE("UploadMesh describes position, normal and uv to the vertex array")
{
    Platform::OpenGLGraphicsDevice device;
    const Renderer::Mesh mesh = Renderer::UploadMesh(device, Quad());
    const GLuint vao = device.GetVertexArray(mesh.vertexArray)->id;

    const auto attrib = [vao](GLuint index, GLenum pname) {
        GLint value = -1;
        glGetVertexArrayIndexediv(vao, index, pname, &value);
        return value;
    };

    // Same locations as mesh.vert: 0 = position, 1 = normal, 2 = uv
    CHECK(attrib(0, GL_VERTEX_ATTRIB_ARRAY_SIZE) == 3);
    CHECK(attrib(1, GL_VERTEX_ATTRIB_ARRAY_SIZE) == 3);
    CHECK(attrib(2, GL_VERTEX_ATTRIB_ARRAY_SIZE) == 2);
    CHECK(attrib(0, GL_VERTEX_ATTRIB_RELATIVE_OFFSET) == 0);
    CHECK(attrib(1, GL_VERTEX_ATTRIB_RELATIVE_OFFSET) == 12);
    CHECK(attrib(2, GL_VERTEX_ATTRIB_RELATIVE_OFFSET) == 24);

    Renderer::DestroyMesh(device, mesh);
}

GL_TEST_CASE("DestroyMesh releases the vertex array and both buffers")
{
    Platform::OpenGLGraphicsDevice device;
    const Renderer::Mesh mesh = Renderer::UploadMesh(device, Quad());

    Renderer::DestroyMesh(device, mesh);

    CHECK(device.GetVertexArray(mesh.vertexArray) == nullptr);
    CHECK(device.GetBuffer(mesh.vertexBuffer) == nullptr);
    CHECK(device.GetBuffer(mesh.indexBuffer) == nullptr);
}
