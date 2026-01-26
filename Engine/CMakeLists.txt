# Engine/CMakeLists.txt

project(Engine)

add_library(Engine STATIC)

add_subdirectory("vendor/glad")

target_include_directories(Engine PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
)

file(GLOB_RECURSE SOURCES
        "src/**/*.cpp"
        "src/**/*.hpp")

target_sources(Engine PRIVATE ${SOURCES})

target_link_libraries(Engine
        PUBLIC
        glfw
        glm
        spdlog
        PRIVATE
        glad
)

target_compile_options(Engine PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Werror -pedantic -Wold-style-cast>
)





