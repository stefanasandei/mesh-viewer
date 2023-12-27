cmake_minimum_required(VERSION 3.27)
project(lib)

add_library(lib INTERFACE)

# glfw
add_subdirectory(glfw)
target_link_libraries(lib INTERFACE glfw)
target_include_directories(lib INTERFACE glfw/include)

# glm
add_subdirectory(glm)
target_link_libraries(lib INTERFACE glm)
target_include_directories(lib INTERFACE glm)

# stb
target_include_directories(lib INTERFACE stb)

# vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(lib INTERFACE glfw Vulkan::Vulkan)
target_include_directories(lib INTERFACE ${Vulkan_INCLUDE_DIRS})

# vk-bootstrap
add_subdirectory(vk-bootstrap)
target_link_libraries(lib INTERFACE vk-bootstrap::vk-bootstrap)
target_include_directories(lib INTERFACE vk-bootstrap/src)

# vma
target_include_directories(lib INTERFACE vma/include)

# imgui
add_library(imgui STATIC imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_vulkan.cpp)
target_link_libraries(lib INTERFACE imgui)
target_include_directories(lib INTERFACE imgui)
target_include_directories(imgui PUBLIC imgui glfw/include ${Vulkan_INCLUDE_DIRS})
target_include_directories(lib INTERFACE imgui/backends)

# fastgltf
add_subdirectory(fastgltf)
target_link_libraries(lib INTERFACE fastgltf)
target_include_directories(lib INTERFACE fastgltf/include)
