//
// Created by genin on 06/10/2026.
// Path: Engine/src/Engine/Renderer/Mesh.cpp
//

#include <Engine/Renderer/Mesh.hpp>

GEF::Renderer::Mesh GEF::Renderer::UploadMesh(GraphicsDevice& device,
                                              const MeshData& data)
{
    auto ibo = device.CreateBuffer({ .type = BufferType::Index,
                                     .usage = BufferUsage::Static,
                                     .size = static_cast<uint32_t>(sizeof(
                                         uint32_t)) * static_cast<
                                         uint32_t>(data.indices.
                                         size()),
                                     .data = data.indices.data() });

    auto vbo = device.CreateBuffer({ .type = BufferType::Vertex,
                                     .usage = BufferUsage::Static,
                                     .size = static_cast<uint32_t>(sizeof(
                                         Vertex)) * static_cast<uint32_t>(data.
                                         vertices.
                                         size()),
                                     .data = data.vertices.data() });

    auto vao = device.CreateVertexArray({
        .vertexBuffers = {
            { .buffer = vbo,
              .layout = { { ShaderDataType::Float3, "a_Position" },
                          { ShaderDataType::Float3, "a_Normal" },
                          { ShaderDataType::Float2, "a_UV" } } },
        },
        .indexBuffer = ibo });

    return Mesh{ vbo, ibo, vao, static_cast<uint32_t>(data.indices.size()) };
}

void GEF::Renderer::DestroyMesh(GraphicsDevice& device, const Mesh& mesh)
{
    device.DestroyVertexArray(mesh.vertexArray);
    device.DestroyBuffer(mesh.indexBuffer);
    device.DestroyBuffer(mesh.vertexBuffer);
}
