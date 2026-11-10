# Changelog

All notable changes to GEF are listed here. Versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- `PointLight` and `SpotLight`, any number of each light type.
- Shader storage buffers (`BufferType::ShaderStorage`, `CommandList::BindShaderStorageBuffer`).
- Temporary Reinhard tone mapping in the lit shader.
- `SubmitStats::cpuSubmitTimeMs` and `gpuFrameTimeMs`: CPU time spent issuing the commands, and
  GPU time between two timestamp queries around them. Shown in the demo's overlay and as two
  columns of the benchmark.

### Changed

- `SceneRenderer(GraphicsDevice&)` creates and owns the lit shader, now embedded in the library at
  build time (`Engine/embed/shaders`, `cmake/Embed.cmake`). Games no longer ship `lit.vert` and
  `lit.frag`. The constructor taking a `ShaderHandle` is still there for custom shaders.
- **Breaking**: lights are oriented by their `Transform` (local -Z); `DirectionalLight::direction`
  is removed.
- **Breaking**: the ambient color moves to `Scene::Environment`, a new argument of
  `SceneRenderer::Render`.
- `LightingData.hpp` is renamed `Renderer/Layout/LitShaderLayout.hpp`.

## [0.1.0] - 2026-11-02

First release: a small rendering engine on OpenGL 4.5, driven by an ECS.

### Engine

- The game owns `main`: `GEF::Engine` runs any type satisfying the `GEF::Game` concept
  (`OnUpdate`, `OnRender`, optional `OnInit`, `OnImGui`, `OnShutdown`). No base class, no
  singletons; services are passed through `EngineContext`.
- GLFW window, polled input, event bus (immediate and queued events), logging with spdlog,
  asserts, asset paths relative to the executable.

### Rendering

- `GraphicsDevice` owns every GPU resource (buffers, vertex arrays, shaders, pipelines); the game
  holds generation-checked handles.
- OpenGL 4.5 backend using direct state access.
- Recorded `CommandList`: self-contained draw packets with 64-bit sort keys, sorted and replayed
  at the end of the frame through a state cache that skips redundant binds. Draw and bind counters
  exposed in `SubmitStats`.
- Pipelines with depth test and face culling, uniform buffers (std140).
- GPU instancing: per-instance transform and material, one draw per mesh.
- Blinn-Phong lighting with one directional light, gamma-correct output.
- OBJ loading with vertex deduplication.
- Free-fly perspective camera.
- Frame capture: single screenshots and PNG sequences at a fixed time step.

### Scene

- `Transform`, `MeshRenderer` and `DirectionalLight` components on
  [libecs](https://github.com/Samshiba/libecs) v0.1.0.
- `SceneRenderer` turns the registry into one instanced draw per mesh.

### Tools

- Demo application with an ImGui overlay (entity count, instancing and VSync toggles, light
  direction, frame times, draw and bind counters).
- `GEF_Bench`: reproducible benchmark with frame time percentiles, results in
  `docs/benchmark-results.md`.
- 127 tests with doctest, 57 of them on a real OpenGL context.
- CI on MSVC, GCC 14 and Clang 18 (Debug and Release, warnings as errors), OpenGL tests on Mesa
  under Linux, FetchContent consumer check.
- Usable as a subproject (`GEF::Engine`, `GEF_BUILD_*` options).
