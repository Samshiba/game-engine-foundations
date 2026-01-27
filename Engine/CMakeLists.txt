# Engine/CMakeLists.txt

project(Engine)

add_library(Engine STATIC)

add_subdirectory("vendor/glad")

file(GLOB_RECURSE ENGINE_SOURCES
        "src/**/*.cpp"
        "src/**/*.h"
        "src/**/*.hpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
        "include/**/*.h"
        "include/**/*.hpp"
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

        $<$<CONFIG:Debug>:GEF_DEBUG GEF_ENABLE_ASSERTS GEF_ENABLE_LOGGING>
        $<$<CONFIG:Profile>:GEF_PROFILE GEF_ENABLE_LOGGING>
        $<$<CONFIG:Release>:GEF_RELEASE>
)





