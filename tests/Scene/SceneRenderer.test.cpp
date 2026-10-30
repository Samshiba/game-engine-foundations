//
// Path: tests/Scene/SceneRenderer.test.cpp
//
// The SceneRenderer draws a libecs registry through the CommandList, one
// instanced draw per mesh. The CommandList only records: each draw becomes a
// packet holding the state bound at that moment, so the tests read the
// packets to check *what* the renderer asks for. Submit is only called where
// the executed draw count matters. Meshes, uniform and instance buffers are
// real GPU objects.
//

#include <Engine/Scene/SceneRenderer.hpp>

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <span>
#include <vector>

#include <doctest.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <libecs/core/registry/Registry.hpp>

#include <Engine/Core/FileSystem.hpp>
#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/LightingData.hpp>
#include <Engine/Renderer/Mesh.hpp>
#include <Engine/Scene/Components.hpp>

#include "../Platform/OpenGL/GLTestContext.hpp"

using namespace GEF;
using namespace GEF::Renderer;
using namespace GEF::Scene;
using Registry = libecs::core::registry::Registry;
using DrawPacket = CommandList::DrawPacket;

namespace
{
    // A flat polygon with `triangles` triangles: meshes are told apart by
    // their index count (3 per triangle)
    MeshData Fan(uint32_t triangles)
    {
        MeshData data;
        data.vertices.push_back({ { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0 } });
        for (uint32_t i = 0; i <= triangles; ++i)
        {
            const float angle = float(i) * 0.5f;
            data.vertices.push_back(
                { { glm::cos(angle), glm::sin(angle), 0 }, { 0, 0, 1 }, {} });
        }
        for (uint32_t i = 0; i < triangles; ++i)
        {
            data.indices.push_back(0);
            data.indices.push_back(i + 1);
            data.indices.push_back(i + 2);
        }
        return data;
    }

    // A device, the real lit shader and a SceneRenderer, per test
    struct SceneFixture
    {
        Platform::OpenGLGraphicsDevice device;
        ShaderHandle shader;
        std::optional<SceneRenderer> renderer;
        std::vector<Mesh> meshes;
        Registry registry;
        Camera camera;
        CommandList cmd;

        SceneFixture()
        {
            const auto dir = std::filesystem::path(GEF_SOURCE_DIR)
                / "Application" / "assets" / "shaders";
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
            renderer.reset(); // its batches use the meshes' buffers
            for (const Mesh& mesh : meshes)
                DestroyMesh(device, mesh);
            device.DestroyShader(shader);
        }

        SceneFixture(const SceneFixture&) = delete;
        SceneFixture& operator=(const SceneFixture&) = delete;

        Mesh AddMesh(uint32_t triangles)
        {
            meshes.push_back(UploadMesh(device, Fan(triangles)));
            return meshes.back();
        }

        // One frame: a fresh list, then the renderer records into it
        void Render()
        {
            cmd.Reset();
            renderer->Render(registry, camera, cmd);
        }

        std::span<const DrawPacket> Draws() const
        {
            return cmd.GetDrawPackets();
        }

        // What actually reached the GPU
        uint32_t SubmittedDrawCalls()
        {
            device.Submit(cmd);
            return device.GetLastSubmitStats().drawCalls;
        }

        void AddRenderable(const Transform& transform,
                           const MeshRenderer& meshRenderer)
        {
            const auto entity = registry.CreateEntity();
            registry.EmplaceComponent<Transform>(entity, transform);
            registry.EmplaceComponent<MeshRenderer>(entity, meshRenderer);
        }

        template <typename T>
        T ReadUniforms(uint32_t binding) const
        {
            T value{};
            REQUIRE(!Draws().empty());
            const BufferHandle buffer = Draws()[0].uniformBuffers[binding];
            REQUIRE(device.GetBuffer(buffer) != nullptr);
            glGetNamedBufferSubData(device.GetBuffer(buffer)->id, 0, sizeof(T),
                                    &value);
            return value;
        }

        // The instances a draw reads: the buffer behind attribute 3 (a_Model),
        // the first per-instance attribute, on binding 1 of the draw's VAO
        std::vector<InstanceData> ReadInstances(const DrawPacket& draw) const
        {
            const Platform::GLVertexArray* vao =
                device.GetVertexArray(draw.vertexArray);
            REQUIRE(vao != nullptr);
            // No DSA query for an attribute's buffer: bind the VAO and ask
            GLint buffer = 0;
            glBindVertexArray(vao->id);
            glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
                                &buffer);
            glBindVertexArray(0);
            REQUIRE(buffer != 0);

            std::vector<InstanceData> instances(draw.instanceCount);
            glGetNamedBufferSubData(
                static_cast<GLuint>(buffer),
                static_cast<GLintptr>(draw.baseInstance * sizeof(InstanceData)),
                static_cast<GLsizeiptr>(instances.size()
                                        * sizeof(InstanceData)),
                instances.data());
            return instances;
        }
    };

    bool Near(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::all(glm::lessThan(glm::abs(a - b), glm::vec3(1e-4f)));
    }
} // namespace

GL_TEST_CASE("An empty scene draws nothing")
{
    SceneFixture scene;

    scene.Render();

    CHECK(scene.Draws().empty());
}

GL_TEST_CASE("Entities sharing a mesh are drawn in ONE instanced draw")
{
    SceneFixture scene;
    const Mesh mesh = scene.AddMesh(2);
    for (int i = 0; i < 25; ++i)
        scene.AddRenderable({ .position = { float(i), 0, 0 } },
                            { .mesh = mesh });

    scene.Render();

    REQUIRE(scene.Draws().size() == 1);
    CHECK(scene.Draws()[0].indexCount == mesh.indexCount);
    CHECK(scene.Draws()[0].instanceCount == 25);
}

GL_TEST_CASE("One draw per mesh, with the right instance count each")
{
    SceneFixture scene;
    const Mesh small = scene.AddMesh(1); // 3 indices
    const Mesh large = scene.AddMesh(4); // 12 indices
    for (int i = 0; i < 10; ++i)
        scene.AddRenderable({}, { .mesh = small });
    for (int i = 0; i < 3; ++i)
        scene.AddRenderable({}, { .mesh = large });

    scene.Render();

    // Batch iteration order is unspecified: index by index count
    REQUIRE(scene.Draws().size() == 2);
    std::map<uint32_t, uint32_t> instancesByIndexCount;
    for (const auto& draw : scene.Draws())
        instancesByIndexCount[draw.indexCount] = draw.instanceCount;
    CHECK(instancesByIndexCount[3] == 10);
    CHECK(instancesByIndexCount[12] == 3);
}

GL_TEST_CASE("Entities missing a Transform or a MeshRenderer are not drawn")
{
    SceneFixture scene;
    const Mesh mesh = scene.AddMesh(1);

    const auto onlyTransform = scene.registry.CreateEntity();
    scene.registry.EmplaceComponent<Transform>(onlyTransform, Transform{});

    const auto onlyMesh = scene.registry.CreateEntity();
    scene.registry.EmplaceComponent<MeshRenderer>(onlyMesh,
                                                  MeshRenderer{ .mesh = mesh });

    scene.AddRenderable({}, { .mesh = mesh });

    scene.Render();

    REQUIRE(scene.Draws().size() == 1);
    CHECK(scene.Draws()[0].instanceCount == 1);
}

GL_TEST_CASE("One pipeline, Frame and Light uniform buffers on their bindings")
{
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    REQUIRE(scene.Draws().size() == 1);
    const DrawPacket& draw = scene.Draws()[0];
    CHECK(scene.device.GetPipeline(draw.pipeline) != nullptr);
    // Per-object data moved to the instance buffer: only per-frame UBOs left
    for (const uint32_t binding :
         { UniformBinding::Frame, UniformBinding::Light })
    {
        CAPTURE(binding);
        CHECK(scene.device.GetBuffer(draw.uniformBuffers[binding]) != nullptr);
    }
}

GL_TEST_CASE("The per-frame uniform buffers hold the camera and the light")
{
    SceneFixture scene;
    scene.camera.position = { 1, 2, 3 };

    const auto sun = scene.registry.CreateEntity();
    scene.registry.EmplaceComponent<DirectionalLight>(
        sun,
        DirectionalLight{ .direction = { 0, -2, 0 },
                          .color = { 1.0f, 0.5f, 0.25f },
                          .intensity = 2.0f });
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    const auto frame = scene.ReadUniforms<FrameUniforms>(UniformBinding::Frame);
    CHECK(frame.viewProjection == scene.camera.GetViewProjection());
    CHECK(Near(glm::vec3(frame.cameraPosition), { 1, 2, 3 }));

    const auto light = scene.ReadUniforms<LightUniforms>(UniformBinding::Light);
    CHECK(Near(glm::vec3(light.direction), { 0, -1, 0 }));
    CHECK(Near(glm::vec3(light.color), { 2.0f, 1.0f, 0.5f }));
}

GL_TEST_CASE("The instance buffer holds each entity's transform and material")
{
    SceneFixture scene;
    const Transform transform{ .position = { 4, 5, 6 },
                               .rotation = glm::angleAxis(glm::radians(30.0f),
                                                          glm::vec3(0, 1, 0)),
                               .scale = { 2, 1, 0.5f } };
    scene.AddRenderable(transform,
                        { .mesh = scene.AddMesh(1),
                          .material = { .albedo = { 0.5f, 0.5f, 0.5f },
                                        .shininess = 64.0f,
                                        .specularStrength = 0.25f } });

    scene.Render();

    REQUIRE(scene.Draws().size() == 1);
    const auto instances = scene.ReadInstances(scene.Draws()[0]);
    REQUIRE(instances.size() == 1);
    const InstanceData& instance = instances[0];

    CHECK(instance.model == transform.GetMatrix());

    // The normal matrix is the inverse transpose of the model's 3x3
    const glm::mat3 expected =
        glm::transpose(glm::inverse(glm::mat3(transform.GetMatrix())));
    for (int column = 0; column < 3; ++column)
        CHECK(Near(instance.normalMatrix[column], expected[column]));

    // Albedo converted from sRGB to linear, then shininess and specular
    CHECK(Near(glm::vec3(instance.albedo), glm::vec3(glm::pow(0.5f, 2.2f))));
    CHECK(instance.params.x == doctest::Approx(64.0f));
    CHECK(instance.params.y == doctest::Approx(0.25f));
}

GL_TEST_CASE(
    "The instance buffer grows past its capacity without losing a frame")
{
    SceneFixture scene;
    const Mesh mesh = scene.AddMesh(1);

    // First frame: a few instances, the batch gets its initial capacity
    for (int i = 0; i < 3; ++i)
        scene.AddRenderable({ .position = { float(i), 0, 0 } },
                            { .mesh = mesh });
    scene.Render();
    REQUIRE(scene.Draws().size() == 1);
    REQUIRE(scene.Draws()[0].instanceCount == 3);

    // Far more than any reasonable initial capacity
    constexpr int Total = 3 + 1000;
    for (int i = 3; i < Total; ++i)
        scene.AddRenderable({ .position = { float(i), 0, 0 } },
                            { .mesh = mesh });
    scene.Render();

    // The frame where the buffer grows must still draw every instance
    REQUIRE(scene.Draws().size() == 1);
    CHECK(scene.Draws()[0].instanceCount == Total);

    // ...with the data of all of them, in the new buffer
    const auto instances = scene.ReadInstances(scene.Draws()[0]);
    REQUIRE(instances.size() == Total);
    float maxX = -1.0f;
    for (const InstanceData& instance : instances)
        maxX = glm::max(maxX, instance.model[3].x);
    CHECK(maxX == doctest::Approx(float(Total - 1)));
}

GL_TEST_CASE("Instancing on: one draw per mesh, reading from the first slot")
{
    SceneFixture scene;
    const Mesh mesh = scene.AddMesh(1);
    for (int i = 0; i < 10; ++i)
        scene.AddRenderable({}, { .mesh = mesh });

    REQUIRE(scene.renderer->IsInstancingEnabled()); // the default
    scene.Render();

    REQUIRE(scene.Draws().size() == 1);
    CHECK(scene.Draws()[0].instanceCount == 10);
    CHECK(scene.Draws()[0].baseInstance == 0);
    CHECK(scene.SubmittedDrawCalls() == 1);
}

GL_TEST_CASE("Instancing off: one draw per entity, each reading its own slot")
{
    // Same data, same shader, same instance buffer: only the number of draws
    // changes. That is what isolates the cost of a draw call.
    SceneFixture scene;
    const Mesh mesh = scene.AddMesh(1);
    constexpr uint32_t Count = 10;
    for (uint32_t i = 0; i < Count; ++i)
        scene.AddRenderable({ .position = { float(i), 0, 0 } },
                            { .mesh = mesh });

    scene.renderer->SetInstancingEnabled(false);
    scene.Render();

    REQUIRE(scene.Draws().size() == Count);
    CHECK(scene.SubmittedDrawCalls() == Count);

    // Each draw reads its own slot, so each draw sees a different entity.
    // (Slot order follows the view's iteration order, which libecs leaves
    // unspecified (it walks backwards): don't assume slot i = entity i.)
    std::vector<bool> slotDrawn(Count, false);
    std::vector<bool> entityDrawn(Count, false);
    for (const auto& draw : scene.Draws())
    {
        CHECK(draw.instanceCount == 1);
        REQUIRE(draw.baseInstance < Count);
        slotDrawn[draw.baseInstance] = true;

        // Entity i was placed at x = i
        const auto instance = scene.ReadInstances(draw);
        REQUIRE(instance.size() == 1);
        const auto entity = static_cast<uint32_t>(instance[0].model[3].x);
        REQUIRE(entity < Count);
        entityDrawn[entity] = true;
    }
    // Every slot read once, every entity drawn once: if baseInstance were
    // ignored, all draws would read slot 0 and show the same entity
    for (uint32_t i = 0; i < Count; ++i)
    {
        CAPTURE(i);
        CHECK(slotDrawn[i]);
        CHECK(entityDrawn[i]);
    }
}

GL_TEST_CASE(
    "The draw call count follows the toggle from one frame to the next")
{
    SceneFixture scene;
    const Mesh a = scene.AddMesh(1);
    const Mesh b = scene.AddMesh(2);
    for (int i = 0; i < 5; ++i)
        scene.AddRenderable({}, { .mesh = a });
    for (int i = 0; i < 3; ++i)
        scene.AddRenderable({}, { .mesh = b });

    scene.Render();
    CHECK(scene.SubmittedDrawCalls() == 2); // one per mesh

    scene.renderer->SetInstancingEnabled(false);
    scene.Render();
    CHECK(scene.SubmittedDrawCalls() == 8); // one per entity

    scene.renderer->SetInstancingEnabled(true);
    scene.Render();
    CHECK(scene.SubmittedDrawCalls() == 2); // reset each frame
}
