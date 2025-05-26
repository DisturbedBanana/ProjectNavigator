cmake_minimum_required(VERSION 3.15)
project(ProjectNavigator CXX)

set(CMAKE_CXX_STANDARD 17)

# GLFW
add_subdirectory(libs/glfw)

# ImGui sources (core + backends)
file(GLOB IMGUI_SOURCES
    libs/imgui/*.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

# Include directories for imgui library
target_include_directories(imgui PUBLIC 
    libs/imgui 
    libs/imgui/backends
    libs/glfw/include  # <-- added this line
)

# Your executable
add_executable(ProjectNavigator src/main.cpp)

# Include directories for your executable
target_include_directories(ProjectNavigator PRIVATE 
    libs/imgui 
    libs/imgui/backends 
    libs/glfw/include
)

# Link dependencies
find_package(OpenGL REQUIRED)
target_link_libraries(ProjectNavigator PRIVATE imgui glfw OpenGL::GL)
