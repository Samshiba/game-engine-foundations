# tests/CMakeLists.txt

add_executable(GEF_Tests
        "main.cpp"
        "Assets/ObjLoader.test.cpp"
        "Core/FileSystem.test.cpp"
        "Core/PublicApi.test.cpp"
        "Renderer/Camera.test.cpp"
        "Renderer/ResourcePool.test.cpp"
        "Platform/OpenGL/LitShader.test.cpp"
        "Platform/OpenGL/OpenGLGraphicsDevice.test.cpp"
        "Platform/OpenGL/OpenGLMesh.test.cpp"
        "Platform/OpenGL/OpenGLPipeline.test.cpp"
        "Platform/OpenGL/OpenGLShader.test.cpp"
        "Platform/OpenGL/OpenGLUniformBuffer.test.cpp"
)

target_include_directories(GEF_Tests
        # SYSTEM: don't report warnings coming from the vendored doctest header
        SYSTEM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/doctest"

        # Unit tests may reach the engine's private headers
        PRIVATE
        "${PROJECT_SOURCE_DIR}/Engine/src"
)

target_link_libraries(GEF_Tests PRIVATE Engine glad glfw)

gef_set_warnings(GEF_Tests)

# Lets tests read the real project files (e.g. the game's shaders), so a test
# checks what actually ships, not a copy that could drift
target_compile_definitions(GEF_Tests PRIVATE
        GEF_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

add_test(NAME GEF_Tests COMMAND GEF_Tests)
