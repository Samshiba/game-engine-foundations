# tests/CMakeLists.txt

add_executable(GEF_Tests
        "main.cpp"
        "Renderer/ResourcePool.test.cpp"
)

target_include_directories(GEF_Tests
        # SYSTEM: don't report warnings coming from the vendored doctest header
        SYSTEM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/doctest"

        # Unit tests may reach the engine's private headers
        PRIVATE
        "${PROJECT_SOURCE_DIR}/Engine/src"
)

target_link_libraries(GEF_Tests PRIVATE Engine)

gef_set_warnings(GEF_Tests)

add_test(NAME GEF_Tests COMMAND GEF_Tests)
