# Cgravity
Gravity simulation  in C++ and OpenGL

![demo](./demo.png)

## Dependencies
    - OpenGL APIs: gl:core=3.3
    - glad 2.0.8
    - C++17


## Getting Started

### 1. Clone the Repository
Clone the main repository to your local machine:
```bash
git clone <repository-url>
cd <repository-folder>
```

### 2. Initialize Submodules
The project uses Git submodules, you must initialize and clone them after cloning the main repository:
```bash
git submodule update --init --recursive
```
*(Alternatively, you can clone everything at once using `git clone --recurse-submodules <repository-url>`)*

### 3. Build and Run
Configure the project with CMake, compile, and run the executable:
```bash
make run
```
## Usage
 - use up/down arraw key to change the speed
 - use w/a/s/d keys to move within the space
 - use mouse movments to change camera view
 - use mouse scroll to zoom in or out
 - use mouse left click to pause/resume the demo

