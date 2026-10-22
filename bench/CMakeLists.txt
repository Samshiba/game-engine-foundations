# bench/CMakeLists.txt

add_executable(GEF_Bench "src/Benchmark.cpp")

target_link_libraries(GEF_Bench PRIVATE GEF_Engine)

gef_set_warnings(GEF_Bench)

# Same assets as the game (lit shaders, cube and bunny), copied next to the
# executables. Both land in bin/, so reuse the Application's copy step.
add_dependencies(GEF_Bench ApplicationAssets)
