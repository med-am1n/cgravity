# Cgravity
Gravity simulation  in C++ and OpenGL

![demo](./demo.png)

## Dependencies
    - OpenGL APIs: gl:core=3.3
    - glad 2.0.8
    - C++17

## Usage
clone/add 3rd party libraries :
   - ``` git submodule add https://github.com/g-truc/glm external/glm ```
   - ``` git submodule add https://github.com/ocornut/imgui external/imgui ```

- mkdir & cd to build and run: ``` cmake .. ```
    or from root directory: ``` cmake -S . -B build/ ```


- from root directory run : ``` make && ./prog ```

- then :
    - use up/down arraw key to change the speed
    - use w/a/s/d keys to move within the space
    - use mouse movments to change camera view
    - use mouse scroll to zoom in or out
    - use mouse left click to pause/resume the demo

