cmake_minimum_required(VERSION 3.10)
project(cgravity)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)

add_executable(prog
    src/main.cpp
    src/stb_image.cpp
    external/glad/src/gl.c
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
