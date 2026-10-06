//
// Created by genin on 05/10/2026.
// Path: Engine/include/Engine/Renderer/Mesh.hpp
//

#pragma once

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/Handle.hpp>

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>
#include <cstdint>

#include <glm/glm.hpp>


namespace GEF::Renderer
{
    struct Vertex // 32 bytes, interleaved
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;

        friend bool operator==(const Vertex&, const Vertex&) = default;
    };

    struct MeshData // CPU side
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct Mesh // GPU side: handles only
    {
        BufferHandle vertexBuffer;
        BufferHandle indexBuffer;
        VertexArrayHandle vertexArray;
        uint32_t indexCount = 0;
    };

    std::optional<MeshData> ParseObj(std::string_view objText); // pure parsing
    std::optional<MeshData> LoadObj(const std::filesystem::path& path);
    // ReadTextFile + ParseObj
    Mesh UploadMesh(GraphicsDevice& device, const MeshData& data);
    void DestroyMesh(GraphicsDevice& device, const Mesh& mesh);
    // VAO first, then buffers
}