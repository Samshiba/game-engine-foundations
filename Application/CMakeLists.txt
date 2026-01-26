# Application/CMakeLists.txt

project(Application)

add_executable(Application "src/App.cpp")

target_link_libraries(Application PRIVATE Engine)