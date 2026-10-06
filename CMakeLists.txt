# CMakeList.txt : Top-level CMake project file

cmake_minimum_required(VERSION 3.28)
project(GameEngineFoundations VERSION 0.1.0 LANGUAGES C CXX)

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set up Build Types
get_property(multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (multi)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Profile;Release" CACHE STRING "" FORCE)
else ()
    if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Profile" "Release")
    endif ()
endif ()

# Profile = optimized build with debug info. CMake has no defaults for custom
# configurations, so it borrows the RelWithDebInfo flags (for dependencies too).
foreach (lang C CXX)
    set(CMAKE_${lang}_FLAGS_PROFILE "${CMAKE_${lang}_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
endforeach ()
set(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
set(CMAKE_STATIC_LINKER_FLAGS_PROFILE "${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
set(CMAKE_MAP_IMPORTED_CONFIG_PROFILE RelWithDebInfo Release)

# Same MSVC runtime for every target, dependencies included
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Strict warnings, applied to our own targets only
function(gef_set_warnings target)
    target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /permissive- /Zc:preprocessor /utf-8>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
            -Wall -Wextra -Wpedantic -Werror
            -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual>
    )
endfunction()

# Setup libraries (SYSTEM: their headers don't trigger our warnings)
include(FetchContent)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 SYSTEM)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.3 SYSTEM)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.17.0 SYSTEM)
FetchContent_Declare(tinyobjloader
        GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
        GIT_TAG v2.0.0rc13
        SYSTEM)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw glm spdlog tinyobjloader)

# Projects
add_subdirectory(Engine)
add_subdirectory(Application)

option(GEF_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})
if (GEF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
