# Application/CMakeLists.txt

project(Application)

add_executable(Application "src/Sandbox.cpp" "src/FreeFlyController.cpp")

target_link_libraries(Application PRIVATE Engine)

gef_set_warnings(Application)

# Copy assets/ next to the executable, so the game finds them relative to the
# exe whatever the working directory. A custom target (not a POST_BUILD step):
# it runs on every build, so editing a shader alone, without relinking the
# exe, still updates the copy. Only changed files are copied.
add_custom_target(ApplicationAssets
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Application>/assets"
        COMMENT "Copying Application assets"
)
add_dependencies(Application ApplicationAssets)
