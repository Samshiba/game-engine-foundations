# Application/CMakeLists.txt

project(Application)

add_executable(Application "src/Sandbox.cpp")

target_link_libraries(Application PRIVATE Engine)

gef_set_warnings(Application)
