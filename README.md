# opengl-init


## future use refrence, 

i may forget, nah sure i will so :


## Usage
- update CmakeLists.txt project to the actual project name:
    project("Opengl-init") = > project(Project-name)

- update root_directory.h path to the actual project path:
    logl_root = "/Users/ahmed/Desktop/repos/c/opengl-init"; = > static const char* logl_root = "path/to/project";


- mkdir & cd to build and run: ``` cmake .. ```
    or from root directory: ``` cmake -S . -B build/ ```


- cd to build then: make then: ``` ./prog ```


- for vscode to avoid some lint errors :

    add includes/libraries/externals directories to the includePath in c_cpp_properties.json in .vscode dir:
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/external/glad/include",
                "${workspaceFolder}/includes",
                "${workspaceFolder}/external/glm",
                "/opt/homebrew/include"
            ], 