# tests/consumer/CMakeLists.txt
#
# A separate project that uses GEF the way the README tells users to, with
# FetchContent. SOURCE_DIR points at this checkout instead of the GitHub
# repository, so CI checks the code being tested.
#
#   cmake -S tests/consumer -B build-consumer
#   cmake --build build-consumer

cmake_minimum_required(VERSION 3.28)
project(GEF_Consumer LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(gef SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
FetchContent_MakeAvailable(gef)

# As a subproject, GEF builds its library only
foreach (target Application GEF_Bench GEF_Tests)
    if (TARGET ${target})
        message(FATAL_ERROR "${target} should not be built when GEF is a subproject")
    endif ()
endforeach ()

add_executable(consumer "main.cpp")
target_link_libraries(consumer PRIVATE GEF::Engine)
