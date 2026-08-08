cmake_minimum_required(VERSION 3.10)
project(cgravity)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)

set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

# 2. Add the source files here so the linker can build them for arm64
add_executable(prog
    src/main.cpp
    src/stb_image.cpp
    external/glad/src/gl.c
    ${IMGUI_SOURCES}
)

target_include_directories(prog PRIVATE
    external/glad/include
    external/glm
    external/imgui
    includes
    
)

target_link_libraries(prog
    glfw
    "-framework OpenGL"
)
