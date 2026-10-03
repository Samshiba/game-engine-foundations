# Engine/CMakeLists.txt

project(Engine)

add_library(Engine STATIC)

add_subdirectory("vendor/glad")

set(ENGINE_SOURCES
        "src/Commons.hpp"

        # Core
        "src/Engine/Core/Application.cpp"
        "src/Engine/Core/Log.cpp"
        "src/Engine/Core/KeyCodes.hpp"
        "src/Engine/Core/Input.hpp"
        "src/Engine/Core/Input.cpp"
        "src/Engine/Core/Window.cpp"

        # Event
        "src/Engine/Event/EventBus.cpp"

        # Platform
        "src/Engine/Platform/OpenGL/OpenGlContext.hpp"
        "src/Engine/Platform/OpenGL/OpenGlContext.cpp"
        "src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.hpp"
        "src/Engine/Platform/OpenGL/OpenGLGraphicsDevice.cpp"
        "src/Engine/Platform/OpenGL/OpenGlIndexBuffer.hpp"
        "src/Engine/Platform/OpenGL/OpenGlIndexBuffer.cpp"
        "src/Engine/Platform/OpenGL/OpenGlVertexBuffer.hpp"
        "src/Engine/Platform/OpenGL/OpenGlVertexBuffer.cpp"
        "src/Engine/Platform/OpenGL/OpenGlVertexArray.hpp"
        "src/Engine/Platform/OpenGL/OpenGlVertexArray.cpp"
        "src/Engine/Platform/OpenGL/OpenGLCommandList.hpp"

        # GLFW handles the windowing on every desktop platform
        "src/Engine/Platform/Glfw/GlfwWindow.hpp"
        "src/Engine/Platform/Glfw/GlfwWindow.cpp"

        # Renderer
        "src/Engine/Renderer/GraphicsContext.hpp"
        "src/Engine/Renderer/GraphicsContext.cpp"
        "src/Engine/Renderer/GraphicsDevice.cpp"
)

set(ENGINE_HEADERS
        "include/Engine.hpp"

        # Utils
        "include/Engine/Utils/FNV-1a.hpp"

        # Core
        "include/Engine/Core/Application.hpp"
        "include/Engine/Core/EntryPoint.hpp"
        "include/Engine/Core/Base.hpp"
        "include/Engine/Core/Log.hpp"
        "include/Engine/Core/Window.hpp"
        "include/Engine/Core/Assert.hpp"

        # Event
        "include/Engine/Event/Event.hpp"
        "include/Engine/Event/ApplicationEvent.hpp"
        "include/Engine/Event/EventBus.hpp"

        # Renderer
        "include/Engine/Renderer/RendererAPI.hpp"
        "include/Engine/Renderer/GraphicsDevice.hpp"
        "include/Engine/Renderer/Buffer.hpp"
        "include/Engine/Renderer/IndexBuffer.hpp"
        "include/Engine/Renderer/VertexBuffer.hpp"
        "include/Engine/Renderer/VertexArray.hpp"
        "include/Engine/Renderer/CommandList.hpp"
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
        glfw
        glm
        spdlog

        PRIVATE
        glad
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
        $<$<CONFIG:Release>:GEF_RELEASE_BUILD>

        # SPDLOG
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>
        $<$<CONFIG:Profile>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
        $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERROR>
)