cmake_minimum_required(VERSION 3.27)
project(mesh_viewer)

set(CMAKE_CXX_STANDARD 23)

add_executable(mesh_viewer src/main.cpp
        src/platform/window.h
        src/util/std.hpp
        src/util/error.cpp
        src/platform/window.cpp
        src/gfx/gfx.hpp
        src/gfx/renderer.hpp
        src/gfx/vulkan_utils.hpp
        src/gfx/context.hpp
        src/global.hpp
        src/gfx/renderer.cpp
        src/global.cpp
        src/gfx/context.cpp
        src/memory/deletion_queue.hpp
        src/memory/deletion_queue.cpp
        src/gfx/swapchain.hpp
        src/gfx/swapchain.cpp
        src/gfx/allocated_image.hpp
        src/gfx/allocated_image.cpp
        src/gfx/vma_impl.cpp
        src/gfx/imgui_layer.hpp
        src/gfx/imgui_layer.cpp
        src/gfx/pipeline_builder.hpp
        src/gfx/pipeline_builder.cpp
        src/gfx/shader.hpp
        src/gfx/shader.cpp
        src/gfx/allocated_buffer.hpp
        src/gfx/allocated_buffer.cpp
        src/gfx/mesh.hpp
        src/gfx/mesh.cpp)

add_subdirectory(lib)

target_link_libraries(mesh_viewer PRIVATE lib)

include_directories(src)
