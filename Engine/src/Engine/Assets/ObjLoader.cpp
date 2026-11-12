//
// Created by genin on 06/10/2026.
// Path: Engine/src/Engine/Assets/ObjLoader.cpp
//

#include <Engine/Assets/ObjLoader.hpp>

#include <unordered_map>

#include <tiny_obj_loader.h>

#include <Engine/Core/FileSystem.hpp>
#include <Engine/Core/Log.hpp>
#include <Engine/Utils/HashCombine.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// Hash implementation for unordered_map use
template <>
struct std::hash<GEF::Renderer::Vertex>
{
    std::size_t operator()(const GEF::Renderer::Vertex& v) const noexcept
    {
        return GEF::Utils::HashAll(v.position, v.normal, v.uv, v.tangent);
    }
};

std::optional<GEF::Renderer::MeshData>
GEF::Assets::ParseObj(std::string_view objText)
{
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;

    tinyobj::ObjReader reader;

    if (!reader.ParseFromString(std::string(objText), "", config))
    {
        if (!reader.Error().empty())
        {
            GEF_ENGINE_ERROR("TinyObjReader: {}", reader.Error());
        }
        return std::nullopt;
    }

    if (!reader.Warning().empty())
    {
        GEF_ENGINE_WARN("TinyObjReader: {}", reader.Warning());
    }

    if (reader.GetShapes().empty())
    {
        return std::nullopt;
    }

    Renderer::MeshData meshData{};
    std::unordered_map<Renderer::Vertex, uint32_t> vertexMap{};

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();

    bool hasNormals = false;

    const auto isInRange = [](int index, size_t count) {
        return index >= 0 && static_cast<size_t>(index) < count;
    };

    // ----- Create Vertex -----
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Renderer::Vertex vertex{};

            // Position
            if (!isInRange(index.vertex_index, attrib.vertices.size() / 3))
            {
                GEF_ENGINE_ERROR("OBJ: vertex index {} out of range",
                                 index.vertex_index);
                return std::nullopt;
            }
            vertex.position = { attrib.vertices[3 * index.vertex_index + 0],
                                attrib.vertices[3 * index.vertex_index + 1],
                                attrib.vertices[3 * index.vertex_index + 2] };

            // Normal
            if (index.normal_index != -1)
            {
                if (!isInRange(index.normal_index, attrib.normals.size() / 3))
                {
                    GEF_ENGINE_ERROR("OBJ: normal index {} out of range",
                                     index.normal_index);
                    return std::nullopt;
                }
                vertex.normal = { attrib.normals[3 * index.normal_index + 0],
                                  attrib.normals[3 * index.normal_index + 1],
                                  attrib.normals[3 * index.normal_index + 2] };
                hasNormals = true;
            }

            // UV
            if (index.texcoord_index != -1)
            {
                if (!isInRange(index.texcoord_index,
                               attrib.texcoords.size() / 2))
                {
                    GEF_ENGINE_ERROR("OBJ: texcoord index {} out of range",
                                     index.texcoord_index);
                    return std::nullopt;
                }
                vertex.uv = { attrib.texcoords[2 * index.texcoord_index + 0],
                              attrib.texcoords[2 * index.texcoord_index + 1] };
            }

            if (!vertexMap.contains(vertex))
            {
                vertexMap[vertex] =
                    static_cast<uint32_t>(meshData.vertices.size());
                meshData.vertices.push_back(vertex);
            }

            meshData.indices.push_back(vertexMap[vertex]);
        }
    }

    // ----- Calculate Normal if not in file -----
    if (!hasNormals)
    {
        for (size_t i = 0; i + 2 < meshData.indices.size(); i += 3)
        {
            Renderer::Vertex& a = meshData.vertices[meshData.indices[i + 0]];
            Renderer::Vertex& b = meshData.vertices[meshData.indices[i + 1]];
            Renderer::Vertex& c = meshData.vertices[meshData.indices[i + 2]];

            const glm::vec3 faceNormal =
                glm::cross(b.position - a.position, c.position - a.position);

            a.normal += faceNormal;
            b.normal += faceNormal;
            c.normal += faceNormal;
        }

        for (Renderer::Vertex& vertex : meshData.vertices)
            vertex.normal = glm::normalize(vertex.normal);
    }

    // ----- Calculate Tangent -----
    std::vector<glm::vec3> bitangents;
    bitangents.resize(meshData.indices.size());
    for (size_t i = 0; i + 2 < meshData.indices.size(); i += 3)
    {
        const uint32_t ia = meshData.indices[i + 0];
        const uint32_t ib = meshData.indices[i + 1];
        const uint32_t ic = meshData.indices[i + 2];

        Renderer::Vertex& a = meshData.vertices[ia];
        Renderer::Vertex& b = meshData.vertices[ib];
        Renderer::Vertex& c = meshData.vertices[ic];

        const glm::vec3 edge1 = b.position - a.position;
        const glm::vec3 edge2 = c.position - a.position;
        const glm::vec2 deltaUV1 = b.uv - a.uv;
        const glm::vec2 deltaUV2 = c.uv - a.uv;

        const float det = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        if (glm::abs(det) < 1e-8f)
            continue;
        const float f = 1.0f / det;

        const glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
        const glm::vec3 bitangent =
            f * (deltaUV1.x * edge2 - deltaUV2.x * edge1);

        a.tangent += glm::vec4(tangent, 0.0f);
        b.tangent += glm::vec4(tangent, 0.0f);
        c.tangent += glm::vec4(tangent, 0.0f);

        bitangents[ia] += bitangent;
        bitangents[ib] += bitangent;
        bitangents[ic] += bitangent;
    }

    for (Renderer::Vertex& vertex : meshData.vertices)
    {
        glm::vec3 T = xyz(vertex.tangent)
            - glm::dot(xyz(vertex.tangent), vertex.normal) * vertex.normal;
        if (glm::dot(T, T) < 1e-12f)
        {
            // no UVs: any axis perpendicular to N (no effect with the flat
            // default map)
            const glm::vec3 axis = glm::abs(vertex.normal.x) < 0.9f
                ? glm::vec3(1, 0, 0)
                : glm::vec3(0, 1, 0);
            T = glm::cross(vertex.normal, axis);
        }
        T = glm::normalize(T);
        float w = glm::dot(glm::cross(vertex.normal, T),
                           bitangents[&vertex - meshData.vertices.data()])
                < 0.0f
            ? -1.0f
            : 1.0f;
        vertex.tangent = glm::vec4(T, w);
    }

    return meshData;
}

std::optional<GEF::Renderer::MeshData>
GEF::Assets::LoadObj(const std::filesystem::path& path)
{
    auto string = FileSystem::ReadTextFile(path);
    if (!string)
        return std::nullopt;

    return ParseObj(string.value());
}
