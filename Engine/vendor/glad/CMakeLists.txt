# Engine/vendor/glad/CMakeLists.txt

project(Glad)

add_library(glad STATIC "src/glad.c")

target_include_directories(glad PUBLIC "include")

if(MSVC)
    target_compile_options(glad PRIVATE /w)
else()
    target_compile_options(glad PRIVATE -w)
endif()