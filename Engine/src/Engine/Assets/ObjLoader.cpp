//
// Created by genin on 06/10/2026.
// Path: Engine/src/Engine/Assets/ObjLoader.cpp
//

#include <Engine/Assets/ObjLoader.hpp>
#include <Engine/Core/FileSystem.hpp>
#include <Engine/Core/Log.hpp>

#include <unordered_map>

#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// Hash implementation for unordered_map use
template <>
struct std::hash<GEF::Renderer::Vertex>
{
    std::size_t operator()(const GEF::Renderer::Vertex& v) const noexcept
    {
        std::size_t seed = 0x19BD75E6;
        seed ^= (seed << 6) + (seed >> 2) + 0x0C4A02F0 + std::hash<
            glm::vec3>{}(v.position);
        seed ^= (seed << 6) + (seed >> 2) + 0x6AAFFEDC + std::hash<
            glm::vec3>{}(v.normal);
        seed ^= (seed << 6) + (seed >> 2) + 0x68196E62 + std::hash<
            glm::vec2>{}(v.uv);
        return seed;
    }
};

std::optional<GEF::Renderer::MeshData> GEF::Assets::ParseObj(
    std::string_view objText)
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

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Renderer::Vertex vertex{};

            if (!isInRange(index.vertex_index, attrib.vertices.size() / 3))
            {
                GEF_ENGINE_ERROR("OBJ: vertex index {} out of range",
                                 index.vertex_index);
                return std::nullopt;
            }
            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.normal_index != -1)
            {
                if (!isInRange(index.normal_index, attrib.normals.size() / 3))
                {
                    GEF_ENGINE_ERROR("OBJ: normal index {} out of range",
                                     index.normal_index);
                    return std::nullopt;
                }
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
                hasNormals = true;
            }

            if (index.texcoord_index != -1)
            {
                if (!isInRange(index.texcoord_index,
                               attrib.texcoords.size() / 2))
                {
                    GEF_ENGINE_ERROR("OBJ: texcoord index {} out of range",
                                     index.texcoord_index);
                    return std::nullopt;
                }
                vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            if (!vertexMap.contains(vertex))
            {
                vertexMap[vertex] = static_cast<uint32_t>(meshData.vertices.
                    size());
                meshData.vertices.push_back(vertex);
            }

            meshData.indices.push_back(vertexMap[vertex]);
        }
    }

    if (!hasNormals)
    {
        for (size_t i = 0; i + 2 < meshData.indices.size(); i += 3)
        {
            Renderer::Vertex& a = meshData.vertices[meshData.indices[i + 0]];
            Renderer::Vertex& b = meshData.vertices[meshData.indices[i + 1]];
            Renderer::Vertex& c = meshData.vertices[meshData.indices[i + 2]];

            const glm::vec3 faceNormal = glm::cross(
                b.position - a.position, c.position - a.position);

            a.normal += faceNormal;
            b.normal += faceNormal;
            c.normal += faceNormal;
        }

        for (Renderer::Vertex& vertex : meshData.vertices)
            vertex.normal = glm::normalize(vertex.normal);
    }

    return meshData;
}

std::optional<GEF::Renderer::MeshData> GEF::Assets::LoadObj(
    const std::filesystem::path& path)
{
    auto string = FileSystem::ReadTextFile(path);
    if (!string)
        return std::nullopt;

    return ParseObj(string.value());
}
