# Engine/CMakeLists.txt

project(Engine)

add_library(Engine STATIC)

add_subdirectory("vendor/glad")

set(ENGINE_SOURCES
        "src/Commons.hpp"

        # Core
        "src/Engine/Core/Engine.cpp"
        "src/Engine/Core/Log.cpp"
        "src/Engine/Core/Input.cpp"
        "src/Engine/Core/Window.cpp"
        "src/Engine/Core/FileSystem.cpp"

        # Event
        "src/Engine/Event/EventBus.cpp"

        # Platform
        "src/Engine/Platform/OpenGL/OpenGlContext.hpp"
        "src/Engine/Platform/OpenGL/OpenGlContext.cpp"
        "src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp"
        "src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.cpp"
        "src/Engine/Platform/OpenGL/OpenGLCommandList.hpp"

        # GLFW handles the windowing on every desktop platform
        "src/Engine/Platform/Glfw/GlfwWindow.hpp"
        "src/Engine/Platform/Glfw/GlfwWindow.cpp"

        # Renderer
        "src/Engine/Renderer/GraphicsContext.hpp"
        "src/Engine/Renderer/GraphicsContext.cpp"
        "src/Engine/Renderer/GraphicsDevice.cpp"
        "src/Engine/Renderer/ResourcePool.hpp"
        "src/Engine/Renderer/Camera.cpp"
        "src/Engine/Renderer/Mesh.cpp"

        # Assets
        "src/Engine/Assets/ObjLoader.cpp"

        # Scene
        "src/Engine/Scene/SceneRenderer.cpp"
)

set(ENGINE_HEADERS
        "include/Engine.hpp"

        # Utils
        "include/Engine/Utils/FNV-1a.hpp"

        # Core
        "include/Engine/Core/Engine.hpp"
        "include/Engine/Core/Input.hpp"
        "include/Engine/Core/KeyCodes.hpp"
        "include/Engine/Core/Base.hpp"
        "include/Engine/Core/Log.hpp"
        "include/Engine/Core/Window.hpp"
        "include/Engine/Core/Assert.hpp"
        "include/Engine/Core/FileSystem.hpp"

        # Event
        "include/Engine/Event/Event.hpp"
        "include/Engine/Event/ApplicationEvent.hpp"
        "include/Engine/Event/ApplicationEvent.inl"
        "include/Engine/Event/EventBus.hpp"

        # Renderer
        "include/Engine/Renderer/RendererAPI.hpp"
        "include/Engine/Renderer/GraphicsDevice.hpp"
        "include/Engine/Renderer/Buffer.hpp"
        "include/Engine/Renderer/CommandList.hpp"
        "include/Engine/Renderer/Handle.hpp"
        "include/Engine/Renderer/Camera.hpp"
        "include/Engine/Renderer/Mesh.hpp"
        "include/Engine/Renderer/LightingData.hpp"

        # Assets
        "include/Engine/Assets/ObjLoader.hpp"

        # Scene
        "include/Engine/Scene/Components.hpp"
        "include/Engine/Scene/SceneRenderer.hpp"
)

target_sources(Engine PRIVATE
        ${ENGINE_SOURCES}
        ${ENGINE_HEADERS}
)

target_include_directories(Engine
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"

        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(Engine
        PUBLIC
        glm
        spdlog
        libecs::libecs

        PRIVATE
        glfw
        glad
        tinyobjloader
)

# Compile Options (optimization flags come from the build type, see root CMakeLists)
gef_set_warnings(Engine)

# Compile Definitions
target_compile_definitions(Engine PUBLIC
        # Platform
        $<$<PLATFORM_ID:Windows>:GEF_PLATFORM_WINDOWS>
        $<$<PLATFORM_ID:Linux>:GEF_PLATFORM_LINUX>
        $<$<PLATFORM_ID:Darwin>:GEF_PLATFORM_MACOS>

        # Build Type
        $<$<CONFIG:Debug>:GEF_DEBUG_BUILD GEF_ENABLE_ASSERTS GEF_ENABLE_LOGGING>
        $<$<CONFIG:Profile>:GEF_PROFILE_BUILD GEF_ENABLE_LOGGING>
        $<$<CONFIG:Release>:GEF_RELEASE_BUILD GEF_ENABLE_LOGGING>

        # SPDLOG
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>
        $<$<CONFIG:Profile>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
        $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERROR>
)