//
// Path: tests/Core/PublicApi.test.cpp
//
// Compiles the umbrella header on its own: every public header must be
// self-contained and reachable from <Engine.hpp>. A header that forgets an
// include, or pulls a private one, breaks this file's build.
//

#include <Engine.hpp>

#include <doctest.h>

TEST_CASE("The umbrella header exposes the public API")
{
    // Spot checks: one type per module
    CHECK(sizeof(GEF::EngineSpecification) > 0);
    CHECK(sizeof(GEF::Events::EventBus) > 0);
    CHECK(sizeof(GEF::Renderer::Camera) > 0);
    CHECK(sizeof(GEF::Renderer::Mesh) > 0);
    CHECK(sizeof(GEF::Scene::Transform) > 0);
    CHECK(GEF::Assets::ParseObj("").has_value() == false);
}
