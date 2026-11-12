# GEF architecture

This document explains how GEF is organized and what happens during a frame. It describes
v0.1.0. The first part gives the overall picture and a short tour of every module; the second
part goes into the pieces that shape the renderer.

1. [Overview](#overview)
2. [Tour of the modules](#tour-of-the-modules)
3. [The engine and the game](#the-engine-and-the-game)
4. [A frame from start to finish](#a-frame-from-start-to-finish)
5. [GPU resources and handles](#gpu-resources-and-handles)
6. [The command list](#the-command-list)
7. [Submitting a frame](#submitting-a-frame)
8. [From entities to instanced draws](#from-entities-to-instanced-draws)
9. [Lighting](#lighting)
10. [Textures and normal maps](#textures-and-normal-maps)
11. [Frame capture](#frame-capture)
12. [Rules the code relies on](#rules-the-code-relies-on)
13. [Known limits](#known-limits)

## Overview

GEF is a static library (`GEF::Engine`) that a game links against. The game keeps its world in a
libecs registry: entities with a `Transform` and a `MeshRenderer` (which mesh, which material),
and lights (`DirectionalLight`, `PointLight`, `SpotLight`) placed by their own `Transform`. The
engine runs the loop, and on each frame:

1. the game updates its entities (input, movement, gameplay);
2. the scene renderer reads the entities and records draw commands into a command list;
3. the graphics device sorts the commands and turns them into OpenGL calls.

The code is split in layers. Each layer only uses the ones below it:

```mermaid
flowchart TB
    subgraph APP["Game (Application/)"]
        GAME["Sandbox<br/>OnInit, OnUpdate, OnRender, OnImGui, OnShutdown"]
    end

    subgraph SCENE["Scene"]
        SR["SceneRenderer"]
        COMP["Components<br/>Transform, MeshRenderer, Material, lights"]
    end

    subgraph RENDERER["Renderer, independent of the graphics API"]
        CMD["CommandList"]
        DEV["GraphicsDevice interface"]
        MISC["Handles, Mesh, Texture, Camera, FrameCapture"]
    end

    subgraph CORE["Core"]
        ENG["Engine (frame loop)"]
        SVC["Window, Input, EventBus, Log, FileSystem"]
    end

    subgraph PLATFORM["Platform (src/Engine/Platform)"]
        GL["OpenGLGraphicsDevice"]
        WIN["GlfwWindow"]
        GUI["ImGui GLFW/OpenGL backend"]
    end

    GAME --> SR
    GAME --> COMP
    GAME --> ENG
    SR --> CMD
    SR --> DEV
    ENG --> SVC
    ENG --> DEV
    DEV -. implemented by .-> GL
    SVC -. implemented by .-> WIN
    ENG --> GUI
```

The public headers are in `Engine/include/Engine/` and never include OpenGL, glad or GLFW. All
the code that calls those APIs lives in `Engine/src/Engine/Platform/`, behind interfaces
(`Window`, `GraphicsDevice`, `ImGuiBackend`) whose implementation is picked at startup. A game
written against GEF does not depend on OpenGL.

## Tour of the modules

**Core** (`Engine/Core/`)

- `Engine` owns the window, the graphics device and the ImGui backend, and runs the frame loop.
  It is described [below](#the-engine-and-the-game).
- `Window` is an interface; `GlfwWindow` implements it. It creates the OpenGL context, translates
  GLFW callbacks into engine events, and handles VSync, cursor mode and window mode.
- `Input` is read by polling: once per frame it copies the state of every key and mouse button,
  and keeps the previous frame's state, which gives `IsKeyPressed`, `IsKeyJustPressed`,
  `IsKeyJustReleased` and the mouse delta.
- `EventBus` delivers window events (resize, close, keys) to subscribers. `Subscribe` returns an
  id used to `Unsubscribe`.
- `Log` wraps two spdlog loggers, one for the engine and one for the game, behind macros
  (`GEF_ENGINE_INFO`, `GEF_ERROR`, ...). `Assert` provides `GEF_CORE_ASSERT`, active in Debug only.
- `FileSystem` finds the executable's directory and the `assets/` folder next to it, and reads
  text files.

**Renderer** (`Engine/Renderer/`)

- `GraphicsDevice` is the interface to the GPU: create and destroy buffers, vertex arrays,
  textures, shaders and pipelines, update buffers, submit a command list, read the framebuffer
  back.
- `Handle` is the type the device hands out for each resource
  ([details](#gpu-resources-and-handles)).
- `CommandList` records the frame's draws ([details](#the-command-list)).
- `Mesh` holds the vertex format (`Vertex`: position, normal, tangent, UV) and `UploadMesh` /
  `DestroyMesh`, which turn CPU-side `MeshData` into GPU buffers and back.
- `Texture` does the same for images: `UploadTexture` turns CPU-side `ImageData` into a texture,
  in the format the caller picks ([details](#textures-and-normal-maps)).
- `Camera` is a perspective camera defined by a position, yaw and pitch. It computes the view and
  projection matrices.
- `Layout/LitShaderLayout.hpp` is the contract between the C++ side and the lit shader: the
  binding of each uniform block, storage block and texture, the C++ copies of their layouts, and
  the per-instance format (`InstanceData`) ([details](#lighting)).
- `FrameCapture` saves screenshots and frame sequences ([details](#frame-capture)).

**Scene** (`Engine/Scene/`): the ECS components and the `SceneRenderer`, which turns them into
draws ([details](#from-entities-to-instanced-draws)).

**Assets** (`Engine/Assets/`): `LoadObj` reads an OBJ file with tinyobjloader and returns
`MeshData`, merging identical vertices so they are shared by the index buffer, and computes the
tangents. `LoadImage` reads an image with stb_image and returns `ImageData` (RGBA8, bottom row
first, as OpenGL expects). `WritePng` saves one with stb_image_write.

**UI** (`Engine/UI/`): `ImGuiBackend` sets up Dear ImGui for the window and the graphics API. The
game draws its own windows with the ImGui API in `OnImGui`.

**Utils** (`Engine/Utils/`): `FrameTimer` collects the frame times and, every second, computes
their statistics (mean, median, p95, p99, worst) with `FrameStatistics`; the benchmark uses the same
code. `Overloaded` is the usual helper for `std::visit`, and `HashCombine` combines the hashes of
several fields into one, for the keys of hash maps.

**Platform** (`src/Engine/Platform/`): the implementations. `OpenGLGraphicsDevice` and
`OpenGLContext` (which loads OpenGL with glad, checks for version 4.5, and in Debug forwards the
driver's debug messages to the log), `GlfwWindow`, and the ImGui backend for GLFW and OpenGL.

## The engine and the game

There is no base class to inherit from and no global instance. The game's `main` creates both
the engine and the game, and starts the loop:

```cpp
int main()
{
    GEF::Engine engine({ .title = "Sandbox" });
    Sandbox game;
    return engine.Run(game);
}
```

`Run` is a template constrained by the `GEF::Game` concept: the type needs
`OnUpdate(EngineContext&, float dt)` and `OnRender(EngineContext&, CommandList&)`. `OnInit`,
`OnImGui` and `OnShutdown` are optional; `Run` checks for them at compile time with
`if constexpr (requires { ... })`. Since the game's type is known at compile time, there is no
virtual call in the loop.

The game never receives the engine itself. Every callback gets an `EngineContext`, a struct of
references to what a game may use: the window, the input, the graphics device, the event bus and
the frame capture. To give games access to a new engine service, a field is added to that struct.

Ownership is kept simple:

| What                                   | Owned by            | Lives until                                                                   |
|----------------------------------------|---------------------|-------------------------------------------------------------------------------|
| Window, graphics device, ImGui backend | `Engine`            | the engine is destroyed (the window last, since the device needs its context) |
| The frame's `CommandList`              | `Engine`            | the end of the run; it is reset every frame but keeps its memory              |
| GPU objects                            | `GraphicsDevice`    | the game calls `Destroy*` on the handle                                       |
| `SceneRenderer`, meshes, shaders       | the game            | usually created in `OnInit`, destroyed in `OnShutdown`                        |
| Entities and components                | the game's registry | the game destroys them                                                        |

## A frame from start to finish

```mermaid
sequenceDiagram
    autonumber
    participant E as Engine
    participant G as Game
    participant SR as SceneRenderer
    participant C as CommandList
    participant D as GraphicsDevice
    E ->> E: compute dt
    E ->> G: OnUpdate(ctx, dt)
    E ->> C: Reset, then viewport, clear color and clear
    E ->> G: OnRender(ctx, cmd)
    G ->> SR: Render(registry, camera, cmd)
    SR ->> D: UpdateBuffer (per-frame data, instances)
    SR ->> C: Bind..., DrawIndexedInstanced
    E ->> G: OnImGui(ctx)
    E ->> D: Submit(cmd)
    Note over D: sort, then OpenGL calls
    E ->> E: render ImGui, capture if requested
    E ->> E: swap buffers, poll window events
```

Window events are produced while polling, at the end of the frame. The engine handles the ones
that concern it first (close stops the loop, resize updates the viewport size), then passes every
event to the event bus, where the game may have subscribed. The demo subscribes to resize to keep
the camera's aspect ratio right.

## GPU resources and handles

The game never holds an OpenGL id or a pointer to a GPU object. When it creates a resource, the
device stores the object and returns a handle:

```cpp
template <typename Tag>
struct Handle { uint32_t index; uint32_t generation; };

using BufferHandle = Handle<BufferTag>;   // also VertexArrayHandle, TextureHandle, ShaderHandle...
```

The tag only makes each handle a distinct type, so a buffer handle cannot be passed where a
pipeline is expected.

On the device side, each kind of resource is kept in a `ResourcePool`: an array of slots plus a
list of free slots. `index` is the slot. `generation` counts how many times that slot was reused:
destroying a resource increments it. A handle to a destroyed resource therefore no longer matches
its slot, even after a new resource took the place. Using it triggers an assert in Debug and
returns nothing in Release, instead of quietly using the wrong object.

This is the main difference with a design where each resource is a C++ object holding its
OpenGL id and freeing it in its destructor:

- the device decides when GPU objects are freed, which is always at a known point, with a valid
  context;
- handles are 8 bytes and can be copied, compared and hashed, which the command list and the
  scene renderer rely on;
- the API does not mention OpenGL anywhere.

Resources are described with plain structs: `BufferDesc` (type, usage, size, initial data),
`VertexArrayDesc` (vertex buffers with their layout and instance divisor, index buffer),
`TextureDesc` (size, format, mipmaps, pixels), `ShaderDesc` (sources), `PipelineDesc` (shader,
depth test and write, face culling). The OpenGL
device uses Direct State Access (`glCreateBuffers`, `glNamedBufferSubData`,
`glVertexArrayAttribFormat`, ...), which modifies objects directly instead of binding them first.

## The command list

A naive renderer calls OpenGL as soon as the game asks for something. The order of the calls is
then the order in which the game happens to draw, and every draw re-binds its shader, its mesh and
its uniforms even when they did not change.

GEF records first and executes later. While recording, the `CommandList` only stores data:

```cpp
cmd.BindPipeline(pipeline);            // remembered as the current pipeline
cmd.BindVertexArray(mesh.vertexArray); // remembered as the current mesh
cmd.BindUniformBuffer(UniformBinding::Frame, frameBuffer);
cmd.DrawIndexedInstanced(indexCount, instanceCount, firstInstance);
```

`Bind*` calls only update the list's current state. Each draw copies that state into a
`DrawPacket`: pipeline, vertex array, the buffer of each uniform and storage binding, the texture
of each texture unit, and the draw's parameters. A packet does not depend on what was recorded before it, so packets can
be executed
in any order.

Each packet also gets a 64-bit sort key. Sorting the keys puts the packets in the order that
minimizes state changes:

| Bits  | 63 to 60 | 59 to 48 | 47 to 36     | 35 to 20 | 19 to 0  |
|-------|----------|----------|--------------|----------|----------|
| Field | pass     | pipeline | vertex array | depth    | sequence |

- The most expensive change, the pipeline, is in the high bits, so all draws with the same
  pipeline end up together, then grouped by mesh inside it.
- The sequence is the packet's position in the recording. Every key is different, so the sort
  gives the same result on every run, and the key also says which packet it belongs to. Only the
  keys are sorted, the packets stay where they are.
- The pass and depth fields are not used yet (always 0). They are reserved for separating opaque
  and transparent objects and for ordering by distance.
- The field sizes set the limits: 4096 pipelines, 4096 vertex arrays, about a million draws per
  frame. They are checked by asserts.

Viewport, clear color and clear are frame-level commands. They are kept in a separate list and
executed in order, before the draws.

The command list is reused from one frame to the next. `Reset` empties it without releasing its
memory, so once the biggest frame has been seen, recording does not allocate anymore.

## Submitting a frame

`OpenGLGraphicsDevice::Submit` sorts the keys, runs the frame commands, then goes through the
packets in sorted order. It remembers what is currently bound (pipeline, vertex array, uniform
buffer of each binding) and only calls OpenGL for what differs from the previous packet. Binding
a pipeline sets the shader program and its depth and culling state. Then it issues the draw with
`glDrawElementsInstancedBaseInstance`.

What is currently bound is forgotten at the start of every `Submit`. ImGui renders between two
frames and changes OpenGL state without going through the device, so trusting the previous
frame's state would skip binds that are needed.

The device counts what it really did in `SubmitStats`: draw calls, pipeline changes, vertex array
changes and uniform buffer changes. The demo's overlay and the benchmark read them with
`GetLastSubmitStats()`.

It also times the replay twice. `cpuSubmitTimeMs` is the CPU time spent issuing the commands to
the driver (the sort is not included). `gpuFrameTimeMs` is the time the GPU takes between two
timestamp queries placed around the same commands. The GPU time is read a few frames later, when
the result is available, because asking for it right away would make the CPU wait for the GPU.
Comparing the two tells which side limits `Submit`. When the CPU time is the larger, the driver
cannot take the draw calls fast enough. When the GPU time is the larger, the CPU is done long
before the GPU. If both are small next to the frame time, the frame is limited elsewhere, for
example by the scene renderer building and uploading the instance data.

Buffer updates are the exception to "record first": `UpdateBuffer` writes immediately. This works
because each buffer is written once per frame, before `Submit` (see the
[rules](#rules-the-code-relies-on)).

## From entities to instanced draws

The components are plain structs:

| Component          | Content                                                             |
|--------------------|---------------------------------------------------------------------|
| `Transform`        | position, rotation (quaternion), scale                              |
| `MeshRenderer`     | a `Mesh` (handles and index count) and a `Material`                 |
| `DirectionalLight` | color, intensity                                                    |
| `PointLight`       | color, intensity, range                                             |
| `SpotLight`        | color, intensity, range, inner and outer cone half-angles (degrees) |

A `Material` is not a component but a value inside `MeshRenderer`: a color, a shininess and a
specular strength, plus an optional albedo map and normal map (texture handles). The color tints
the albedo map.

A light takes its position and orientation from the `Transform` of its entity, and shines along
its local -Z. The scale is ignored. A light without a `Transform` is not drawn.

`SceneRenderer::Environment` is not a component: there is one per scene, so the game passes it to
`Render` like the camera. It holds the ambient color.

`SceneRenderer::Render(registry, camera, environment, cmd)` does the following each frame:

1. It writes two uniform buffers that are the same for every object: the frame data (camera
   matrices and position, binding 0) and the environment (ambient color, binding 1).
2. It collects the lights into one list per type and uploads each list to its shader storage buffer
   ([details](#lighting)).
3. It goes through every entity that has a `Transform` and a `MeshRenderer`, and adds an
   `InstanceData` to its batch: model matrix, normal matrix, color and specular values. A batch
   is found by its key: the mesh's vertex array and the material's two maps. A missing map is
   replaced by a 1x1 default texture before building the key.
4. For each batch, it uploads the instances to the batch's instance buffer (doubling its size
   when it is too small), binds the batch's textures and records the draw.

Textures are bound once per draw, not per instance, so entities sharing a mesh but not their maps
end up in different batches, each with its own draw.

Instancing means drawing the same mesh several times with one call, each copy reading its own
data. The batch's vertex array combines two buffers: the mesh's vertices, read once per vertex,
and the instance buffer, read once per instance (divisor 1). The vertex shader receives the
position, normal, tangent and UV of the vertex, and the model matrix, normal matrix and material of the
instance it belongs to.

With instancing on, a batch is one draw of `count` instances. With it off, the renderer records
`count` draws of one instance each, starting at instance `i`. Both use the same data and the same
shader, which is why the demo's toggle and the benchmark measure the cost of draw calls alone.

## Lighting

The lit shader (`lit.vert.glsl`, `lit.frag.glsl`, embedded in the library from
`Engine/embed/shaders`) uses the Blinn-Phong model. Each fragment starts from the ambient term,
then adds the diffuse and specular terms of every light: first the directional lights, then the
point lights, then the spot lights. Colors are picked in sRGB, converted to linear on the CPU and
lit in linear space. The CPU multiplies each light's color by its intensity, so the shader receives
the light's radiance directly.

### One shader storage buffer per light type

The number of lights changes with the scene, so the lights do not go in uniform buffers, whose
arrays have a fixed size. Each type has its own shader storage buffer (SSBO, `std430` layout):
a 16-byte header holding the count, then the array of lights.

```
byte:  0        4             16                  16 + n * size
       [count ][ padding     ][ light 0 ][ light 1 ] ...
```

The padding is there because `std430` starts an array of `vec4` structs on a multiple of 16.
The shader loops up to the count, not up to the array's length: a buffer keeps its capacity
when lights are removed, so its end still holds the previous frames' lights.

The SceneRenderer keeps the capacity of each buffer. When a list outgrows it, the buffer is
recreated with twice the capacity before being written, so no frame is lost. The lists are member
vectors cleared each frame, which keeps the steady state free of allocations.

One storage buffer per type, rather than one list of all lights with a type field, keeps each
light struct small and each loop simple. A single list would be the next step for tiled or
clustered shading, which culls lights with one list of indices; the upload code is grouped in
one function so that change stays local.

Uniform and storage buffers have separate binding points in OpenGL: the `Frame` uniform block and
the directional light buffer can both use binding 0.

### Attenuation and cones

Point and spot lights use PBR formula: inverse square of the
distance, multiplied by a window that falls smoothly to exactly 0 at the light's range:

```
attenuation = max(0, 1 - (d / r)^4)^2 / (d^2 + 1)
```

The `+ 1` avoids the division by zero at the light and assumes units are meters. The range only
says where to cut the light off, not how fast it falls off.

A spot light lights a fragment fully inside its inner cone, not at all outside its outer cone,
with a smooth transition between the two. The CPU sends the cosines of the half-angles, so the
shader compares them directly with the cosine between the cone's axis and the fragment, without
an `acos` per fragment.

### Tone mapping and gamma

Light adds up and has no maximum: several lights on one surface easily go above 1. Clamping would
turn every bright area into flat white, so the shader compresses the color with a Reinhard
operator (`c / (c + 1)`), then applies the gamma correction. This is temporary: a later
post-processing pass will render to an HDR target and tone map there.

### Keeping C++ and GLSL in sync

The C++ copies of the blocks in `Layout/LitShaderLayout.hpp` only use `vec4` and `mat4`, which
have the same layout in C++ and in GLSL, and their sizes are checked with `static_assert`. A
size check misses two layouts of the same size with different offsets, so the tests also ask the
driver for the real binding, offset and array stride of every member of the compiled shader, and
compare them with the C++ structs.

## Textures and normal maps

### sRGB or linear: the caller decides

A texture's bytes are either a color encoded for the eye (sRGB) or raw data. An albedo map is a
color: it is created as `SRGBA8`, the bytes are stored as they are, and the GPU decodes them to
linear each time the shader samples them, before filtering. A normal map holds vectors: it is
created as `RGBA8` and read as is. Decoding a normal map as sRGB would bend every normal (the flat
value 0.5 would become about 0.21).

Nothing in an image file says which one it is, so `UploadTexture` takes the format from the
caller, and the same image can be uploaded in either format. Mipmaps are generated by default;
for an sRGB texture, the driver averages them in linear space.

### Default textures

A material without a map still samples one: the scene renderer creates two 1x1 textures at
startup, white for the albedo (the color is multiplied by 1) and (0.5, 0.5, 1) for the normal
(no perturbation once decoded). The shader has a single path, with no branch and no extra uniform
telling it whether a map is there.

### Tangents and the TBN frame

A normal map stores its normals in tangent space, relative to the surface: +Z along the surface
normal, +X along the direction the U coordinate grows (the tangent), +Y along V (the bitangent).
`LoadObj` computes the tangent of each triangle from the differences of its positions and UVs,
adds it to the triangle's vertices, then for each vertex removes the part along the normal and
normalizes it. A vertex stores its tangent as a `vec4`: `w` is +1 or -1 and says whether the UVs
are mirrored, since a mirrored half of a model has its bitangent pointing the other way. A mesh
without UVs gets any tangent perpendicular to its normal; with the flat default map it has no
effect.

The vertex shader brings the normal and the tangent to world space, rebuilds the bitangent as
`cross(N, T) * w`, and passes the TBN matrix to the fragment shader, which turns the normal read
from the map into a world-space normal before lighting.

### Debug views

`SceneRenderer::SetDebugView` makes the lit shader output something other than the lit color,
to check a texture, the UVs or the tangents by eye. The mode goes to the shader in the `flags`
field of the Frame uniforms, and a `switch` at the end of the fragment shader picks the output.
Every fragment of a frame takes the same branch, so the normal render pays almost nothing for it.

| View           | Output                                                                       |
|----------------|------------------------------------------------------------------------------|
| `Albedo`       | the albedo (map times color), gamma-encoded like the source image            |
| `Normal`       | the geometric normal, mapped from [-1, 1] to [0, 1]                          |
| `NormalMap`    | the normal after the normal map, mapped the same way                         |
| `UV`           | U in red, V in green                                                         |
| `Tangent`      | the tangent, mapped from [-1, 1] to [0, 1]                                   |
| `LightingOnly` | the normal render with a mid-gray albedo: only the lights and the normal map |

The data views skip the lighting, the tone mapping and the gamma, so a value can be read straight
from the pixel: a normal facing +Z is (0.5, 0.5, 1), the bluish color of a flat normal map.
`LightingOnly` is the opposite: the whole lighting path runs, only the color of the surface is
removed. It is the easiest way to see whether a normal map does anything. The demo exposes the
views in its overlay.

## Frame capture

The game can ask for a screenshot or a sequence of frames through `ctx.capture`.
`RequestScreenshot` saves the next frame to `screenshots/<timestamp>.png`, and
`RequestSequence(n)` saves the next `n` frames to `screenshots/<timestamp>/frame_0000.png` and
following, ready for ffmpeg.

At the end of the frame, after ImGui and before the swap, the engine calls the capture, which
reads the image back with `GraphicsDevice::ReadFramebuffer`. OpenGL stores images from the
bottom row up, so the rows are flipped before the PNG is written with stb_image_write.

During a sequence, the frames are kept in memory and written at the end, and the engine gives
the game a fixed `dt` of 1/30 s. Reading back and saving frames is slow and irregular; with a
fixed step, each saved frame is exactly 1/30 s after the previous one, and the GIF plays at the
right speed.

## Rules the code relies on

1. **Only the graphics device calls OpenGL**, apart from the ImGui backend. The game and the scene
   renderer use handles and record commands.
2. **A draw packet is self-contained.** This is what allows sorting.
3. **A buffer is written at most once per frame, before `Submit`.** `UpdateBuffer` is
   immediate, so a second write in the same frame would replace the data for every draw of that
   frame.
4. **The bound-state cache only lives during one `Submit`.**
5. **Handles are checked when used**, and a stale handle asserts in Debug.
6. **One command list per frame.** The engine resets it at the start of the frame and submits it
   at the end.

## Known limits

These come from the current design and are the starting points of the
[roadmap](../README.md#roadmap):

- Because buffer updates are immediate, one buffer cannot hold different values for different
  draws of the same frame. A ring buffer for per-frame data, or recorded updates, would lift this.
- Recording happens on one thread. Since packets are self-contained, several threads could each
  fill a list and the lists could be merged before sorting.
- The scene renderer uploads every instance every frame, around 6 MB per frame at 50,000
  entities, even if nothing moved.
- There is a single pass. Shadow maps or post-processing need render targets and several passes
  in a known order, which is what a render graph would organize. Until then, tone mapping is done
  at the end of the lit shader.
- Every fragment evaluates every light, even the ones out of range. Tiled or clustered shading
  would only evaluate the lights that reach each part of the screen.
- Textures are bound per draw, so every combination of mesh and maps is its own batch, with its
  own vertex array and instance buffer. Texture arrays or bindless textures would let instances
  of one mesh use different maps in a single draw.
- The sort key has no texture field. Draws sharing textures still end up next to each other, but
  only as a side effect: each batch creates its own vertex array, so sorting by vertex array
  sorts by (mesh, maps). Two meshes with the same maps are not grouped, and sharing one vertex
  array per mesh would break this. A material field between the pipeline and the vertex array,
  holding a small id per set of textures, would make it explicit.
- Tangents are averaged per vertex with a simple method, not MikkTSpace, the convention most
  tools bake normal maps with. Maps baked from a high-poly model can show slight seams.
- The lighting model is Blinn-Phong. Roughness and metallic maps need a physically based model
  (Cook-Torrance), which belongs with the HDR pass.
- There is one backend. The interfaces were written with a second one in mind, but nothing has
  tested them yet.
