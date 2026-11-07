//
// Path: tests/consumer/main.cpp
//
// Compiles and links a game against GEF::Engine. It is not run: CI runners
// have no display for the window.
//

#include <type_traits>

#include <Engine/Core/Engine.hpp>
#include <Engine/Scene/SceneRenderer.hpp>

struct Game
{
    void OnUpdate(GEF::EngineContext&, float) {}
    void OnRender(GEF::EngineContext&, GEF::Renderer::CommandList&) {}
};

static_assert(GEF::Game<Game>);

// The scene renderer needs nothing but the device: its shader is embedded in the library
static_assert(std::is_constructible_v<GEF::Scene::SceneRenderer, GEF::Renderer::GraphicsDevice&>);

int main(int argc, char**)
{
    // Never true in CI: keeps Engine::Run linked without opening a window
    if (argc > 1)
    {
        GEF::Engine engine({ .title = "Consumer" });
        Game game;
        return engine.Run(game);
    }
    return 0;
}
