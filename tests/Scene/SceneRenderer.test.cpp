//
// Path: tests/Scene/SceneRenderer.test.cpp
//
// The SceneRenderer draws a libecs registry through the CommandList, one
// instanced draw per mesh. The CommandList only records: each draw becomes a
// packet holding the state bound at that moment, so the tests read the
// packets to check *what* the renderer asks for. Submit is only called where
// the executed draw count matters. Meshes, uniform, light storage and instance
// buffers are real GPU objects.
//

#include <Engine/Scene/SceneRenderer.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <vector>

#include <doctest.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <libecs/core/registry/Registry.hpp>

#include <Engine/Core/Embedded.hpp>
#include <Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp>
#include <Engine/Renderer/CommandList.hpp>
#include <Engine/Renderer/Layout/LitShaderLayout.hpp>
#include <Engine/Renderer/Mesh.hpp>
#include <Engine/Scene/Components.hpp>

#include "../Platform/OpenGL/GLTestContext.hpp"

using namespace GEF;
using namespace GEF::Renderer;
using namespace GEF::Scene;
using namespace GEF::Renderer::Layout;
using Registry = libecs::core::registry::Registry;
using DrawPacket = CommandList::DrawPacket;

namespace
{
    // A flat polygon with `triangles` triangles: meshes are told apart by
    // their index count (3 per triangle)
    MeshData Fan(uint32_t triangles)
    {
        MeshData data;
        data.vertices.push_back({ { 0, 0, 0 }, { 0, 0, 1 }, {}, { 0, 0 } });
        for (uint32_t i = 0; i <= triangles; ++i)
        {
            const float angle = float(i) * 0.5f;
            data.vertices.push_back(
                { { glm::cos(angle), glm::sin(angle), 0 }, { 0, 0, 1 }, {}, {} });
        }
        for (uint32_t i = 0; i < triangles; ++i)
        {
            data.indices.push_back(0);
            data.indices.push_back(i + 1);
            data.indices.push_back(i + 2);
        }
        return data;
    }

    // A device and a SceneRenderer (with its embedded lit shader), per test
    struct SceneFixture
    {
        Platform::OpenGLGraphicsDevice device;
        std::optional<SceneRenderer> renderer;
        std::vector<Mesh> meshes;
        Registry registry;
        Camera camera;
        SceneRenderer::Environment environment;
        CommandList cmd;

        SceneFixture()
        {
            renderer.emplace(device);
        }

        ~SceneFixture()
        {
            renderer.reset(); // its batches use the meshes' buffers
            for (const Mesh& mesh : meshes)
                DestroyMesh(device, mesh);
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
            renderer->Render(registry, camera, environment, cmd);
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

        template <typename Light>
        libecs::core::Entity AddLight(const Transform& transform,
                                      const Light& light)
        {
            const auto entity = registry.CreateEntity();
            registry.EmplaceComponent<Transform>(entity, transform);
            registry.EmplaceComponent<Light>(entity, light);
            return entity;
        }

        // A light list as the shader reads it: the header's count, then that
        // many lights right after the 16-byte header
        template <typename GpuLight>
        std::vector<GpuLight> ReadLights(uint32_t binding) const
        {
            REQUIRE(!Draws().empty());
            const BufferHandle handle = Draws()[0].shaderStorageBuffers[binding];
            const Platform::GLBuffer* buffer = device.GetBuffer(handle);
            REQUIRE(buffer != nullptr);
            REQUIRE(buffer->type == BufferType::ShaderStorage);

            LightListHeader header{};
            glGetNamedBufferSubData(buffer->id, 0, sizeof(header), &header);

            // The buffer must hold every light the count announces
            REQUIRE(sizeof(header) + header.count * sizeof(GpuLight)
                    <= buffer->size);

            std::vector<GpuLight> lights(header.count);
            if (header.count > 0)
                glGetNamedBufferSubData(
                    buffer->id, sizeof(header),
                    static_cast<GLsizeiptr>(lights.size() * sizeof(GpuLight)),
                    lights.data());
            return lights;
        }

        // The instances a draw reads: the buffer behind attribute 4 (a_Model),
        // the first per-instance attribute, on binding 1 of the draw's VAO
        std::vector<InstanceData> ReadInstances(const DrawPacket& draw) const
        {
            const Platform::GLVertexArray* vao =
                device.GetVertexArray(draw.vertexArray);
            REQUIRE(vao != nullptr);
            // No DSA query for an attribute's buffer: bind the VAO and ask
            GLint buffer = 0;
            glBindVertexArray(vao->id);
            glGetVertexAttribiv(4, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
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

GL_TEST_CASE("One pipeline, the per-frame buffers on their bindings")
{
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    REQUIRE(scene.Draws().size() == 1);
    const DrawPacket& draw = scene.Draws()[0];
    CHECK(scene.device.GetPipeline(draw.pipeline) != nullptr);
    // Per-object data moved to the instance buffer: only per-frame UBOs left
    for (const uint32_t binding :
         { UniformBinding::Frame, UniformBinding::Environment })
    {
        CAPTURE(binding);
        CHECK(scene.device.GetBuffer(draw.uniformBuffers[binding]) != nullptr);
    }
    // One light list per type, bound even when it is empty
    for (const uint32_t binding :
         { ShaderStorageBinding::DirectionalLight,
           ShaderStorageBinding::PointLight, ShaderStorageBinding::SpotLight })
    {
        CAPTURE(binding);
        const Platform::GLBuffer* buffer =
            scene.device.GetBuffer(draw.shaderStorageBuffers[binding]);
        REQUIRE(buffer != nullptr);
        CHECK(buffer->type == BufferType::ShaderStorage);
    }
}

GL_TEST_CASE("The per-frame uniform buffers hold the camera and the ambient")
{
    SceneFixture scene;
    scene.camera.position = { 1, 2, 3 };
    scene.environment.ambient = { 0.2f, 0.3f, 0.4f };
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    const auto frame = scene.ReadUniforms<FrameUniforms>(UniformBinding::Frame);
    CHECK(frame.viewProjection == scene.camera.GetViewProjection());
    CHECK(Near(glm::vec3(frame.cameraPosition), { 1, 2, 3 }));

    const auto environment =
        scene.ReadUniforms<EnvironmentUniforms>(UniformBinding::Environment);
    CHECK(Near(glm::vec3(environment.ambient), { 0.2f, 0.3f, 0.4f }));
}

GL_TEST_CASE("A scene without lights uploads empty light lists")
{
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    CHECK(scene
              .ReadLights<GpuDirectionalLight>(
                  ShaderStorageBinding::DirectionalLight)
              .empty());
    CHECK(scene.ReadLights<GpuPointLight>(ShaderStorageBinding::PointLight)
              .empty());
    CHECK(scene.ReadLights<GpuSpotLight>(ShaderStorageBinding::SpotLight)
              .empty());
}

GL_TEST_CASE("A directional light points along its Transform's -Z")
{
    SceneFixture scene;
    // -Z turned -90 degrees around X: straight down
    scene.AddLight(Transform{ .rotation = glm::angleAxis(glm::radians(-90.0f),
                                                         glm::vec3(1, 0, 0)) },
                   DirectionalLight{ .color = { 1.0f, 0.5f, 0.25f },
                                     .intensity = 2.0f });
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    const auto lights = scene.ReadLights<GpuDirectionalLight>(
        ShaderStorageBinding::DirectionalLight);
    REQUIRE(lights.size() == 1);
    CHECK(Near(glm::vec3(lights[0].direction), { 0, -1, 0 }));
    // Intensity is folded into the color on the CPU
    CHECK(Near(glm::vec3(lights[0].color), { 2.0f, 1.0f, 0.5f }));
}

GL_TEST_CASE("A point light takes its position from its Transform")
{
    SceneFixture scene;
    // Rotation and scale mean nothing to a point light: only the position
    scene.AddLight(Transform{ .position = { 4, 5, 6 },
                              .rotation = glm::angleAxis(glm::radians(45.0f),
                                                         glm::vec3(0, 1, 0)),
                              .scale = { 3, 3, 3 } },
                   PointLight{ .color = { 0.5f, 1.0f, 0.0f },
                               .intensity = 4.0f,
                               .range = 12.0f });
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    const auto lights =
        scene.ReadLights<GpuPointLight>(ShaderStorageBinding::PointLight);
    REQUIRE(lights.size() == 1);
    CHECK(Near(glm::vec3(lights[0].positionRange), { 4, 5, 6 }));
    CHECK(lights[0].positionRange.w == doctest::Approx(12.0f));
    CHECK(Near(glm::vec3(lights[0].color), { 2.0f, 4.0f, 0.0f }));
}

GL_TEST_CASE("A spot light has a position, a direction and its cone cosines")
{
    SceneFixture scene;
    // Identity rotation: the cone points along -Z
    scene.AddLight(Transform{ .position = { 1, 2, 3 } },
                   SpotLight{ .color = { 1, 1, 1 },
                              .intensity = 3.0f,
                              .range = 20.0f,
                              .innerConeAngle = 30.0f,
                              .outerConeAngle = 45.0f });
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    const auto lights =
        scene.ReadLights<GpuSpotLight>(ShaderStorageBinding::SpotLight);
    REQUIRE(lights.size() == 1);
    const GpuSpotLight& spot = lights[0];
    CHECK(Near(glm::vec3(spot.positionRange), { 1, 2, 3 }));
    CHECK(spot.positionRange.w == doctest::Approx(20.0f));
    CHECK(Near(glm::vec3(spot.direction), { 0, 0, -1 }));
    CHECK(Near(glm::vec3(spot.color), { 3, 3, 3 }));
    // Cosines, so the shader compares dot(-L, axis) without an acos
    CHECK(spot.coneCos.x == doctest::Approx(glm::cos(glm::radians(30.0f))));
    CHECK(spot.coneCos.y == doctest::Approx(glm::cos(glm::radians(45.0f))));
    CHECK(spot.coneCos.x > spot.coneCos.y); // inner cone = larger cosine
}

GL_TEST_CASE("Lights without a Transform are not uploaded")
{
    SceneFixture scene;
    const auto orphan = scene.registry.CreateEntity();
    scene.registry.EmplaceComponent<PointLight>(orphan, PointLight{});
    scene.AddLight(Transform{ .position = { 7, 0, 0 } }, PointLight{});
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    const auto lights =
        scene.ReadLights<GpuPointLight>(ShaderStorageBinding::PointLight);
    REQUIRE(lights.size() == 1);
    CHECK(lights[0].positionRange.x == doctest::Approx(7.0f));
}

GL_TEST_CASE("Each light type goes to its own list")
{
    SceneFixture scene;
    for (int i = 0; i < 2; ++i)
        scene.AddLight(Transform{}, DirectionalLight{});
    for (int i = 0; i < 3; ++i)
        scene.AddLight(Transform{}, PointLight{});
    scene.AddLight(Transform{}, SpotLight{});
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    scene.Render();

    CHECK(scene
              .ReadLights<GpuDirectionalLight>(
                  ShaderStorageBinding::DirectionalLight)
              .size()
          == 2);
    CHECK(scene.ReadLights<GpuPointLight>(ShaderStorageBinding::PointLight)
              .size()
          == 3);
    CHECK(scene.ReadLights<GpuSpotLight>(ShaderStorageBinding::SpotLight)
              .size()
          == 1);
}

GL_TEST_CASE("A light list grows past its capacity without losing a frame")
{
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });

    // First frame: a few lights, the list gets its initial capacity
    for (int i = 0; i < 3; ++i)
        scene.AddLight(Transform{ .position = { float(i), 0, 0 } },
                       PointLight{});
    scene.Render();
    REQUIRE(scene.ReadLights<GpuPointLight>(ShaderStorageBinding::PointLight)
                .size()
            == 3);

    // Far more than any reasonable initial capacity
    constexpr int Total = 3 + 200;
    for (int i = 3; i < Total; ++i)
        scene.AddLight(Transform{ .position = { float(i), 0, 0 } },
                       PointLight{});
    scene.Render();

    // The frame where the buffer grows must hold every light, in the new
    // buffer the draw is bound to
    const auto lights =
        scene.ReadLights<GpuPointLight>(ShaderStorageBinding::PointLight);
    REQUIRE(lights.size() == Total);
    std::vector<bool> seen(Total, false);
    for (const GpuPointLight& light : lights)
    {
        const auto index = static_cast<int>(light.positionRange.x);
        REQUIRE(index >= 0);
        REQUIRE(index < Total);
        seen[index] = true;
    }
    for (int i = 0; i < Total; ++i)
    {
        CAPTURE(i);
        CHECK(seen[i]);
    }
}

GL_TEST_CASE("The light count follows the scene from one frame to the next")
{
    // The buffer keeps its capacity when lights go away: only the count tells
    // the shader how many are real, the rest is last frame's data
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });
    std::vector<libecs::core::Entity> lights;
    for (int i = 0; i < 5; ++i)
        lights.push_back(scene.AddLight(Transform{}, PointLight{}));

    scene.Render();
    REQUIRE(scene.ReadLights<GpuPointLight>(ShaderStorageBinding::PointLight)
                .size()
            == 5);

    for (int i = 0; i < 3; ++i)
        scene.registry.DestroyEntity(lights[i]);
    scene.Render();

    CHECK(scene.ReadLights<GpuPointLight>(ShaderStorageBinding::PointLight)
              .size()
          == 2);
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

GL_TEST_CASE("A renderer given a shader does not destroy it")
{
    Platform::OpenGLGraphicsDevice device;
    const ShaderHandle shader = device.CreateShader(
        { Embedded::LitVertexSource(), Embedded::LitFragmentSource() });
    REQUIRE(shader.IsValid());

    {
        SceneRenderer renderer(device, shader);
    }

    // The shader belongs to whoever created it
    CHECK(device.GetShader(shader) != nullptr);
    device.DestroyShader(shader);
}

// ---------- Debug views ----------
// The fan sits at the origin, facing the default camera (z = 3, looking down
// -Z), and covers the center of GLTestContext's 64x64 window. The pixel read
// back there shows what the debug view outputs.

namespace
{
    std::array<int, 3> CenterPixel(SceneFixture& scene)
    {
        constexpr uint32_t Size = 64; // GLTestContext's hidden window
        CommandList frame;
        frame.SetViewport(0, 0, Size, Size);
        frame.SetClearColor(0, 0, 0, 1);
        frame.Clear();
        scene.device.Submit(frame);
        scene.device.Submit(scene.cmd);

        const auto pixels = scene.device.ReadFramebuffer(Size, Size);
        const std::size_t offset = (std::size_t(Size / 2) * Size + Size / 2) * 4;
        return { pixels[offset], pixels[offset + 1], pixels[offset + 2] };
    }

    bool NearPixel(const std::array<int, 3>& a, const std::array<int, 3>& b)
    {
        for (int i = 0; i < 3; ++i)
            if (std::abs(a[i] - b[i]) > 2)
                return false;
        return true;
    }
} // namespace

GL_TEST_CASE("The debug view reaches the shader through the Frame uniforms")
{
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = scene.AddMesh(1) });
    CHECK(scene.renderer->GetDebugView() == SceneRenderer::DebugView::None);

    scene.renderer->SetDebugView(SceneRenderer::DebugView::Tangent);
    scene.Render();

    const auto frame = scene.ReadUniforms<FrameUniforms>(UniformBinding::Frame);
    CHECK(frame.flags.x
          == static_cast<uint32_t>(SceneRenderer::DebugView::Tangent));
}

GL_TEST_CASE("The albedo debug view shows the material's color, unlit")
{
    SceneFixture scene;
    // No light at all: the normal render would only show the ambient
    scene.environment.ambient = glm::vec3(0.0f);
    scene.AddRenderable(
        {}, { .mesh = scene.AddMesh(12),
              .material = { .albedo = { 0.2f, 0.5f, 1.0f } } });

    scene.renderer->SetDebugView(SceneRenderer::DebugView::Albedo);
    scene.Render();

    // The color as authored (sRGB), like the albedo map shown in an image
    // viewer: the shader works in linear and encodes it back
    const auto pixel = CenterPixel(scene);
    INFO(pixel[0], " ", pixel[1], " ", pixel[2]);
    CHECK(NearPixel(pixel, { 51, 128, 255 }));
}

GL_TEST_CASE("The normal debug view maps the normal from [-1, 1] to [0, 1]")
{
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = scene.AddMesh(12) });

    scene.renderer->SetDebugView(SceneRenderer::DebugView::Normal);
    scene.Render();

    // Normal (0, 0, 1) -> (0.5, 0.5, 1), written as is
    const auto pixel = CenterPixel(scene);
    INFO(pixel[0], " ", pixel[1], " ", pixel[2]);
    CHECK(NearPixel(pixel, { 128, 128, 255 }));
}

GL_TEST_CASE("The normal map debug view shows the flat default map as +Z")
{
    SceneFixture scene;
    scene.AddRenderable({}, { .mesh = scene.AddMesh(12) });

    scene.renderer->SetDebugView(SceneRenderer::DebugView::NormalMap);
    scene.Render();

    // The flat map leaves the normal at (0, 0, 1): same color as above
    const auto pixel = CenterPixel(scene);
    INFO(pixel[0], " ", pixel[1], " ", pixel[2]);
    CHECK(NearPixel(pixel, { 128, 128, 255 }));
}
