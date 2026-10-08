//
// Path: tests/Scene/SceneRenderer.test.cpp
//
// The SceneRenderer draws a libecs registry through the CommandList
// interface. A recording CommandList (a test double) stands in for the
// OpenGL one: it records the calls instead of executing them, so the tests
// check *what* the renderer asks for. Only possible because CommandList is
// an API-independent interface. The uniform buffers are real GPU buffers.
//

#include <doctest.h>

#include "../Platform/OpenGL/GLTestContext.hpp"

#include <Engine/Core/FileSystem.hpp>
#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/LightingData.hpp>
#include <Engine/Scene/Components.hpp>
#include <Engine/Scene/SceneRenderer.hpp>

#include <libecs/core/registry/Registry.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <vector>

using namespace GEF;
using namespace GEF::Renderer;
using namespace GEF::Scene;
using Registry = libecs::core::registry::Registry;

namespace
{
    // Records every call; a draw remembers the vertex array bound before it
    class RecordingCommandList final : public CommandList
    {
    public:
        struct Draw
        {
            VertexArrayHandle vertexArray;
            uint32_t indexCount;
        };

        std::vector<PipelineHandle> pipelines;
        std::map<uint32_t, BufferHandle> uniformBuffers; // binding -> buffer
        std::vector<Draw> draws;

        void SetClearColor(float, float, float, float) override {}
        void Clear() override {}

        void BindVertexArray(VertexArrayHandle handle) override
        {
            boundVertexArray_ = handle;
        }

        void BindPipeline(PipelineHandle handle) override
        {
            pipelines.push_back(handle);
        }

        void BindUniformBuffer(uint32_t binding, BufferHandle handle) override
        {
            uniformBuffers[binding] = handle;
        }

        void DrawIndexed(uint32_t indexCount) override
        {
            draws.push_back({ boundVertexArray_, indexCount });
        }

    private:
        VertexArrayHandle boundVertexArray_;
    };

    // Fake meshes: the renderer only reads their handles and index count
    Mesh FakeMesh(uint32_t vertexArrayIndex, uint32_t indexCount)
    {
        return Mesh{ .vertexArray = VertexArrayHandle{ vertexArrayIndex, 0 },
                     .indexCount = indexCount };
    }

    // A device, the real lit shader and a SceneRenderer, per test
    struct SceneFixture
    {
        Platform::OpenGLGraphicsDevice device;
        ShaderHandle shader;
        std::optional<SceneRenderer> renderer;
        Registry registry;
        Camera camera;
        RecordingCommandList cmd;

        SceneFixture()
        {
            const auto dir = std::filesystem::path(GEF_SOURCE_DIR) /
                "Application" / "assets" / "shaders";
            const auto vertex = FileSystem::ReadTextFile(dir / "lit.vert");
            const auto fragment = FileSystem::ReadTextFile(dir / "lit.frag");
            REQUIRE(vertex.has_value());
            REQUIRE(fragment.has_value());
            shader = device.CreateShader({ *vertex, *fragment });
            REQUIRE(shader.IsValid());
            renderer.emplace(device, shader);
        }

        ~SceneFixture()
        {
            renderer.reset(); // its pipeline references the shader
            device.DestroyShader(shader);
        }

        SceneFixture(const SceneFixture&) = delete;
        SceneFixture& operator=(const SceneFixture&) = delete;

        void Render() { renderer->Render(registry, camera, cmd); }

        libecs::core::Entity AddRenderable(const Transform& transform,
                                           const MeshRenderer& meshRenderer)
        {
            const auto entity = registry.CreateEntity();
            registry.EmplaceComponent<Transform>(entity, transform);
            registry.EmplaceComponent<MeshRenderer>(entity, meshRenderer);
            return entity;
        }

        template <typename T>
        T ReadUniforms(uint32_t binding) const
        {
            T value{};
            const auto it = cmd.uniformBuffers.find(binding);
            REQUIRE(it != cmd.uniformBuffers.end());
            glGetNamedBufferSubData(device.GetBuffer(it->second)->id, 0,
                                    sizeof(T), &value);
            return value;
        }
    };

    bool Near(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::all(glm::lessThan(glm::abs(a - b), glm::vec3(1e-4f)));
    }
}

GL_TEST_CASE("An empty scene draws nothing")
{
    SceneFixture scene;

    scene.Render();

    CHECK(scene.cmd.draws.empty());
}

GL_TEST_CASE("Every entity with a Transform and a MeshRenderer is drawn once")
{
    SceneFixture scene;
    for (int i = 0; i < 25; ++i)
        scene.AddRenderable({ .position = { float(i), 0, 0 } },
                            { .mesh = FakeMesh(3, 36) });

    scene.Render();

    REQUIRE(scene.cmd.draws.size() == 25);
    for (const auto& draw : scene.cmd.draws)
    {
        CHECK(draw.vertexArray == VertexArrayHandle{ 3, 0 });
        CHECK(draw.indexCount == 36);
    }
}

GL_TEST_CASE("Entities missing a Transform or a MeshRenderer are not drawn")
{
    SceneFixture scene;

    const auto onlyTransform = scene.registry.CreateEntity();
    scene.registry.EmplaceComponent<Transform>(onlyTransform, Transform{});

    const auto onlyMesh = scene.registry.CreateEntity();
    scene.registry.EmplaceComponent<MeshRenderer>(
        onlyMesh, MeshRenderer{ .mesh = FakeMesh(1, 3) });

    scene.AddRenderable({}, { .mesh = FakeMesh(2, 6) });

    scene.Render();

    REQUIRE(scene.cmd.draws.size() == 1);
    CHECK(scene.cmd.draws[0].vertexArray == VertexArrayHandle{ 2, 0 });
}

GL_TEST_CASE("Each draw uses its own entity's mesh")
{
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = FakeMesh(1, 3) });
    scene.AddRenderable({}, { .mesh = FakeMesh(2, 6) });

    scene.Render();

    // The view's iteration order is unspecified: compare as a set
    REQUIRE(scene.cmd.draws.size() == 2);
    std::map<uint32_t, uint32_t> indexCountByVertexArray;
    for (const auto& draw : scene.cmd.draws)
        indexCountByVertexArray[draw.vertexArray.index] = draw.indexCount;
    CHECK(indexCountByVertexArray[1] == 3);
    CHECK(indexCountByVertexArray[2] == 6);
}

GL_TEST_CASE("One pipeline and the 4 uniform buffers on their bindings")
{
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = FakeMesh(1, 3) });

    scene.Render();

    CHECK(scene.cmd.pipelines.size() == 1);
    CHECK(scene.cmd.pipelines[0].IsValid());

    // Four distinct, valid buffers, one per binding of lit.vert / lit.frag
    for (const uint32_t binding :
         { UniformBinding::Frame, UniformBinding::Object, UniformBinding::Light,
           UniformBinding::Material })
    {
        CAPTURE(binding);
        REQUIRE(scene.cmd.uniformBuffers.contains(binding));
        CHECK(scene.device.GetBuffer(scene.cmd.uniformBuffers[binding]) !=
              nullptr);
    }
    CHECK(scene.cmd.uniformBuffers.size() == 4);
}

GL_TEST_CASE("The uniform buffers hold the camera, the light and the entity")
{
    SceneFixture scene;
    scene.camera.position = { 1, 2, 3 };

    const auto sun = scene.registry.CreateEntity();
    scene.registry.EmplaceComponent<DirectionalLight>(
        sun, DirectionalLight{ .direction = { 0, -2, 0 },
                               .color = { 1.0f, 0.5f, 0.25f },
                               .intensity = 2.0f });

    const Transform transform{ .position = { 4, 5, 6 },
                               .scale = { 2, 1, 1 } };
    scene.AddRenderable(transform,
                        { .mesh = FakeMesh(1, 3),
                          .material = { .albedo = { 0.5f, 0.5f, 0.5f },
                                        .shininess = 64.0f,
                                        .specularStrength = 0.25f } });

    scene.Render();

    SUBCASE("Frame: view-projection and camera position")
    {
        const auto frame = scene.ReadUniforms<FrameUniforms>(UniformBinding::Frame);
        CHECK(frame.viewProjection == scene.camera.GetViewProjection());
        CHECK(Near(glm::vec3(frame.cameraPosition), { 1, 2, 3 }));
    }

    SUBCASE("Light: normalized direction, color scaled by the intensity")
    {
        const auto light = scene.ReadUniforms<LightUniforms>(UniformBinding::Light);
        CHECK(Near(glm::vec3(light.direction), { 0, -1, 0 }));
        CHECK(Near(glm::vec3(light.color), { 2.0f, 1.0f, 0.5f }));
    }

    SUBCASE("Object: model matrix and the inverse-transpose normal matrix")
    {
        const auto object = scene.ReadUniforms<ObjectUniforms>(UniformBinding::Object);
        CHECK(object.model == transform.GetMatrix());
        const glm::mat3 expected =
            glm::transpose(glm::inverse(glm::mat3(transform.GetMatrix())));
        CHECK(Near(glm::vec3(object.normalMatrix[0]), expected[0]));
        CHECK(Near(glm::vec3(object.normalMatrix[1]), expected[1]));
        CHECK(Near(glm::vec3(object.normalMatrix[2]), expected[2]));
    }

    SUBCASE("Material: albedo converted from sRGB to linear, then the params")
    {
        const auto material =
            scene.ReadUniforms<MaterialUniforms>(UniformBinding::Material);
        const float linear = glm::pow(0.5f, 2.2f); // ~0.218
        CHECK(Near(glm::vec3(material.albedo), glm::vec3(linear)));
        CHECK(material.params.x == doctest::Approx(64.0f));  // shininess
        CHECK(material.params.y == doctest::Approx(0.25f));  // specular strength
    }
}
