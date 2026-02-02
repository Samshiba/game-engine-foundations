# Engine/CMakeLists.txt

project(Engine)

add_library(Engine STATIC)

add_subdirectory("vendor/glad")

set(ENGINE_SOURCES
        "src/Commons.hpp"

        # Utils
        "src/Engine/Utils/FNV-1a.hpp"

        # Core
        "src/Engine/Core/Application.cpp"
        "src/Engine/Core/Log.cpp"
        "src/Engine/Core/Window.hpp"

        # Event
        "src/Engine/Event/Event.hpp"
        "src/Engine/Event/ApplicationEvent.hpp"
        "src/Engine/Event/EventBus.hpp"
        "src/Engine/Event/EventBus.cpp"

        # Platform
        "src/Engine/Platform/Windows/WindowsWindow.hpp"
        "src/Engine/Platform/Windows/WindowsWindow.cpp"
        "src/Engine/Platform/OpenGL/OpenGlContext.hpp"
        "src/Engine/Platform/OpenGL/OpenGlContext.cpp"

        # Renderer
        "src/Engine/Renderer/GraphicsContext.hpp"

)

set(ENGINE_HEADERS
        "include/Engine.hpp"

        # Core
        "include/Engine/Core/Application.hpp"
        "include/Engine/Core/EntryPoint.hpp"
        "include/Engine/Core/Base.hpp"
        "include/Engine/Core/Log.hpp"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_image"
)

target_link_libraries(Engine
        PUBLIC
        glfw
        glm
        spdlog

        PRIVATE
        glad
)

# Compile Options
target_compile_options(Engine PRIVATE
        # --- MSVC Config ---
        $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Od /Zi /RTC1 /MDd>
        $<$<CONFIG:Profile>:/O2 /Zi /MD>
        $<$<CONFIG:Release>:/O2 /MD>
        /W4 /WX /MP
        >

        # --- GCC/Clang Config ---
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Profile>:-O3 -g>
        $<$<CONFIG:Release>:-O3>
        -Wall -Wextra -Werror -pedantic
        >
)

# Compile Definitions
target_compile_definitions(Engine PUBLIC
        $<$<PLATFORM_ID:Windows>:GEF_PLATFORM_WINDOWS>

        $<$<OR:$<CONFIG:Release>,$<CONFIG:Profile>>:NDEBUG>

        $<$<CONFIG:Debug>:GEF_DEBUG_BUILD GEF_ENABLE_ASSERTS GEF_ENABLE_LOGGING>
        $<$<CONFIG:Profile>:GEF_PROFILE_BUILD GEF_ENABLE_LOGGING>
        $<$<CONFIG:Release>:GEF_RELEASE_BUILD>

        # SPDLOG
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>
        $<$<CONFIG:Profile>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
        $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF>
)





