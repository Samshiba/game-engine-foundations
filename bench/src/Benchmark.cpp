//
// Created by genin on 22/10/2026.
// Path: bench/src/Benchmark.cpp
//
// Reproducible rendering benchmark. Every scenario runs in the same
// conditions: fixed 1280x720 non-resizable window, VSync off, fixed camera,
// a warm-up (driver shader compilation, caches, ...) then a timed run. The
// results are written as a Markdown table.
//
// Usage: GEF_Bench [--quick] [--out <file.md>]
//   --quick   short runs on small scenes, to check that everything runs
//             (the numbers are not meaningful)
//

#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>
#include <libecs/core/registry/Registry.hpp>

#include <Engine/Assets/ObjLoader.hpp>
#include <Engine/Core/DedicatedGpu.hpp>
#include <Engine/Core/Engine.hpp>
#include <Engine/Core/FileSystem.hpp>
#include <Engine/Renderer/Camera.hpp>
#include <Engine/Renderer/Mesh.hpp>
#include <Engine/Scene/Components.hpp>
#include <Engine/Scene/SceneRenderer.hpp>
#include <Engine/Utils/FrameStatistics.hpp>

// Laptops: measure the dedicated GPU, not the integrated one
GEF_REQUEST_DEDICATED_GPU();

using namespace GEF;
using namespace GEF::Renderer;

namespace
{
    constexpr uint32_t WindowWidth = 1280;
    constexpr uint32_t WindowHeight = 720;

    struct Options
    {
        float warmupSeconds = 2.0f;
        float measureSeconds = 10.0f;
        std::filesystem::path output = "benchmark-results.md";
        bool quick = false;
    };

    struct Scenario
    {
        std::string_view mesh; // file in assets/models
        uint32_t entities;
        bool instancing;
    };

    // Cubes: the cost of draw calls (12 triangles each).
    // Bunnies: a scene limited by geometry (~69k triangles each).
    constexpr Scenario Scenarios[] = {
        { "cube.obj", 1000, false },
        { "cube.obj", 1000, true },
        { "cube.obj", 10000, false },
        { "cube.obj", 10000, true },
        { "cube.obj", 50000, false },
        { "cube.obj", 50000, true },
        { "stanford-bunny.obj", 1000, false },
        { "stanford-bunny.obj", 1000, true },
        { "stanford-bunny.obj", 10000, false },
        { "stanford-bunny.obj", 10000, true },
    };

    // --quick: a fast sanity check (a few seconds), small scenes only
    constexpr Scenario QuickScenarios[] = {
        { "cube.obj", 100, false },
        { "cube.obj", 100, true },
        { "stanford-bunny.obj", 4, false },
        { "stanford-bunny.obj", 4, true },
    };

    struct Result
    {
        Scenario scenario;
        uint32_t triangles;
        uint32_t drawCalls;
        Utils::FrameStatistics stats;
    };

    Options ParseOptions(int argc, char** argv)
    {
        Options options;
        for (int i = 1; i < argc; ++i)
        {
            const std::string_view arg = argv[i];
            if (arg == "--quick")
            {
                options.quick = true;
                options.warmupSeconds = 0.5f;
                options.measureSeconds = 1.0f;
            }
            else if (arg == "--out" && i + 1 < argc)
            {
                options.output = argv[++i];
            }
            else
            {
                std::cerr << "Unknown argument: " << arg << '\n';
            }
        }
        return options;
    }

    std::optional<ShaderHandle> LoadShader(GraphicsDevice& device,
                                           std::string_view name)
    {
        const std::string base = "shaders/" + std::string(name);
        const auto vs =
            FileSystem::ReadTextFile(FileSystem::AssetPath(base + ".vert"));
        const auto fs =
            FileSystem::ReadTextFile(FileSystem::AssetPath(base + ".frag"));
        if (!vs || !fs)
            return std::nullopt;
        const ShaderHandle shader = device.CreateShader({ *vs, *fs });
        return shader.IsValid() ? std::optional(shader) : std::nullopt;
    }

    // Centers the mesh on the origin and scales it to ~1 unit, so cubes and
    // bunnies fill the same grid cells
    void Normalize(MeshData& data)
    {
        glm::vec3 min(std::numeric_limits<float>::max());
        glm::vec3 max(std::numeric_limits<float>::lowest());
        for (const Vertex& vertex : data.vertices)
        {
            min = glm::min(min, vertex.position);
            max = glm::max(max, vertex.position);
        }
        const glm::vec3 center = (min + max) * 0.5f;
        const float size =
            glm::max(max.x - min.x, glm::max(max.y - min.y, max.z - min.z));
        for (Vertex& vertex : data.vertices)
            vertex.position = (vertex.position - center) / size;
    }

    const char* BuildType()
    {
#if defined(GEF_RELEASE_BUILD)
        return "Release";
#elif defined(GEF_PROFILE_BUILD)
        return "Profile";
#else
        return "Debug";
#endif
    }

    std::string Today()
    {
        const std::time_t now = std::time(nullptr);
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        std::ostringstream out;
        out << std::put_time(&local, "%Y-%m-%d");
        return out.str();
    }

    std::string FormatResults(const std::vector<Result>& results,
                              const AdapterInfo& adapter,
                              const Options& options)
    {
        std::ostringstream out;
        out << std::fixed;
        out << "## GEF rendering benchmark\n\n";
        out << "- **GPU**: " << adapter.renderer << " (" << adapter.vendor
            << ")\n";
        out << "- **Driver**: OpenGL " << adapter.version << '\n';
        out << "- **Build**: " << BuildType() << ", " << WindowWidth << "x"
            << WindowHeight << ", VSync off\n";
        out << "- **Method**: " << std::setprecision(1) << options.warmupSeconds
            << " s warm-up, then " << options.measureSeconds
            << " s measured per scenario (" << Today() << ")\n\n";
        if (options.quick)
            out << "> Quick run: a sanity check, the numbers are not "
                   "meaningful.\n\n";
        if (std::string_view(BuildType()) != "Release")
            out << "> Not a Release build: numbers are not representative.\n\n";

        out << "| Mesh | Triangles/mesh | Entities | Instancing | Draw calls "
               "| FPS | Mean (ms) | p50 (ms) | p99 (ms) | Worst (ms) |\n";
        out << "|---|---:|---:|:---:|---:|---:|---:|---:|---:|---:|\n";
        for (const Result& r : results)
        {
            const std::string_view mesh =
                r.scenario.mesh.substr(0, r.scenario.mesh.find('.'));
            out << "| " << mesh << " | " << r.triangles << " | "
                << r.scenario.entities << " | "
                << (r.scenario.instancing ? "on" : "off") << " | "
                << r.drawCalls << " | " << std::setprecision(1) << r.stats.fps
                << " | " << std::setprecision(2) << r.stats.meanMs << " | "
                << r.stats.p50Ms << " | " << r.stats.p99Ms << " | "
                << r.stats.worstMs << " |\n";
        }
        return out.str();
    }
} // namespace

// A GEF::Game: runs each scenario in turn, then closes the window
class Benchmark
{
public:
    explicit Benchmark(Options options)
        : options_(std::move(options))
        , scenarios_(options_.quick ? std::span<const Scenario>(QuickScenarios)
                                    : std::span<const Scenario>(Scenarios))
    {}

    void OnInit(EngineContext& ctx)
    {
        ctx.window.SetVSync(false);

        const auto shader = LoadShader(ctx.device, "lit");
        if (!shader)
        {
            std::cerr << "Failed to load the lit shader\n";
            ctx.window.Close();
            return;
        }
        shader_ = *shader;
        renderer_.emplace(ctx.device, shader_);

        const auto sun = registry_.CreateEntity();
        registry_.EmplaceComponent<Scene::DirectionalLight>(
            sun,
            Scene::DirectionalLight{ .direction = { -1.0f, -1.0f, -0.5f } });

        camera_.aspectRatio = float(WindowWidth) / float(WindowHeight);
        StartScenario(ctx, 0);
    }

    void OnUpdate(EngineContext& ctx, float dt)
    {
        if (current_ >= scenarios_.size())
            return;

        elapsed_ += dt;
        if (measuring_)
            frameTimesMs_.push_back(dt * 1000.0f);

        if (!measuring_ && elapsed_ >= options_.warmupSeconds)
        {
            measuring_ = true;
            elapsed_ = 0.0f;
        }
        else if (measuring_ && elapsed_ >= options_.measureSeconds)
        {
            FinishScenario();
            if (current_ + 1 < scenarios_.size())
            {
                StartScenario(ctx, current_ + 1);
            }
            else
            {
                current_ = scenarios_.size();
                WriteResults(ctx.device.GetAdapterInfo());
                ctx.window.Close();
            }
        }
    }

    void OnRender(EngineContext&, CommandList& cmd)
    {
        if (renderer_)
            renderer_->Render(registry_, camera_, cmd);
    }

    void OnShutdown(EngineContext& ctx)
    {
        renderer_.reset();
        for (const auto& [name, mesh] : meshes_)
            DestroyMesh(ctx.device, mesh);
        if (shader_.IsValid())
            ctx.device.DestroyShader(shader_);
    }

private:
    const Mesh* GetMesh(EngineContext& ctx, std::string_view name)
    {
        for (const auto& [loaded, mesh] : meshes_)
            if (loaded == name)
                return &mesh;

        auto data = Assets::LoadObj(FileSystem::AssetPath("models")
                                    / std::string(name));
        if (!data)
            return nullptr;
        Normalize(*data);
        meshes_.emplace_back(std::string(name), UploadMesh(ctx.device, *data));
        return &meshes_.back().second;
    }

    void StartScenario(EngineContext& ctx, std::size_t index)
    {
        current_ = index;
        const Scenario& scenario = scenarios_[index];

        for (const libecs::core::Entity entity : entities_)
            registry_.DestroyEntity(entity);
        entities_.clear();

        const Mesh* mesh = GetMesh(ctx, scenario.mesh);
        if (!mesh)
        {
            std::cerr << "Failed to load " << scenario.mesh << '\n';
            ctx.window.Close();
            return;
        }

        // A square grid, 2 units apart, centered on the origin
        const auto side = static_cast<uint32_t>(
            std::ceil(std::sqrt(float(scenario.entities))));
        const float half = float(side - 1);
        for (uint32_t i = 0; i < scenario.entities; ++i)
        {
            const uint32_t x = i % side;
            const uint32_t z = i / side;
            const auto entity = registry_.CreateEntity();
            registry_.EmplaceComponent<Scene::Transform>(
                entity,
                Scene::Transform{ .position = { float(x) * 2.0f - half, 0.0f,
                                                float(z) * 2.0f - half } });
            registry_.EmplaceComponent<Scene::MeshRenderer>(
                entity,
                Scene::MeshRenderer{
                    .mesh = *mesh,
                    .material = { .albedo = { float(x) / float(side),
                                              float(z) / float(side),
                                              0.5f } } });
            entities_.push_back(entity);
        }

        // Same framing for every scenario: the whole grid, seen from above
        camera_.position = { 0.0f, half * 0.9f + 2.0f, half * 1.6f + 4.0f };
        camera_.yaw = -90.0f;
        camera_.pitch = -30.0f;
        camera_.farPlane = glm::max(1000.0f, half * 6.0f);

        renderer_->SetInstancingEnabled(scenario.instancing);
        triangles_ = mesh->indexCount / 3;

        measuring_ = false;
        elapsed_ = 0.0f;
        frameTimesMs_.clear();

        std::cout << "[" << index + 1 << "/" << scenarios_.size() << "] "
                  << scenario.mesh << " x" << scenario.entities
                  << ", instancing " << (scenario.instancing ? "on" : "off")
                  << std::endl;
    }

    void FinishScenario()
    {
        results_.push_back(
            { .scenario = scenarios_[current_],
              .triangles = triangles_,
              .drawCalls = renderer_->GetDrawCallCount(),
              .stats = Utils::ComputeFrameStatistics(frameTimesMs_) });
    }

    void WriteResults(const AdapterInfo& adapter) const
    {
        const std::string table = FormatResults(results_, adapter, options_);
        std::cout << '\n' << table;

        std::ofstream file(options_.output, std::ios::binary);
        file << table;
        std::cout << "\nResults written to "
                  << std::filesystem::absolute(options_.output).string()
                  << '\n';
    }

    Options options_;
    std::span<const Scenario> scenarios_;
    libecs::core::registry::Registry registry_;
    Camera camera_;
    ShaderHandle shader_;
    std::optional<Scene::SceneRenderer> renderer_;
    std::vector<std::pair<std::string, Mesh>> meshes_;
    std::vector<libecs::core::Entity> entities_;

    std::size_t current_ = 0;
    bool measuring_ = false;
    float elapsed_ = 0.0f;
    uint32_t triangles_ = 0;
    std::vector<float> frameTimesMs_;
    std::vector<Result> results_;
};

int main(int argc, char** argv)
{
    const Options options = ParseOptions(argc, argv);

    Engine engine(
        { .title = "GEF Benchmark",
          .width = WindowWidth,
          .height = WindowHeight,
          .flags = Window::FLAGS_VISIBLE | Window::FLAGS_RESIZE_DISABLED });
    Benchmark benchmark(options);
    return engine.Run(benchmark);
}
