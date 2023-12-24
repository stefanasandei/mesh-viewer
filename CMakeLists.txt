cmake_minimum_required(VERSION 3.27)
project(mesh_viewer)

set(CMAKE_CXX_STANDARD 23)

add_executable(mesh_viewer src/main.cpp
        src/platform/window.h
        src/util/std.hpp
        src/util/error.cpp
        src/platform/window.cpp)

add_subdirectory(lib)

target_link_libraries(mesh_viewer PRIVATE lib)

include_directories(src)
