//
// Created by genin on 05/10/2026.
// Path: Engine/include/Engine/Renderer/Mesh.hpp
//

#pragma once

#include <Engine/Renderer/GraphicsDevice.hpp>
#include <Engine/Renderer/Handle.hpp>

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
        static BufferLayout Layout();
    };

    struct MeshData // CPU side
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct Mesh // GPU side: handles only
    {
        BufferHandle vertexBuffer{};
        BufferHandle indexBuffer{};
        VertexArrayHandle vertexArray{};
        uint32_t indexCount = 0;
    };

    struct InstanceData
    {
        glm::mat4 model; // attributes 3-6  (one vec4 per column)
        glm::mat3 normalMatrix; // attributes 7-9  (one vec3 per column)
        glm::vec3 albedo; // attribute 10
        glm::vec2 params; // attribute 11

        static BufferLayout Layout();
    };

    Mesh UploadMesh(GraphicsDevice& device, const MeshData& data);
    void DestroyMesh(GraphicsDevice& device, const Mesh& mesh);
}

inline GEF::Renderer::BufferLayout GEF::Renderer::Vertex::Layout()
{
    return { { ShaderDataType::Float3, "a_Position" },
             { ShaderDataType::Float3, "a_Normal" },
             { ShaderDataType::Float2, "a_UV" } };
}

inline GEF::Renderer::BufferLayout GEF::Renderer::InstanceData::Layout()
{
    return { { ShaderDataType::Mat4, "a_Model" },
             { ShaderDataType::Mat3, "a_NormalMatrix" },
             { ShaderDataType::Float3, "a_Albedo" },
             { ShaderDataType::Float2, "a_Params" } };
}
