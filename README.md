# CUDA Path Tracer

A basic path tracer written in C++20 and CUDA with a naive component system.

Usage:

```
cuda-pathtracer <path-to-obj-file> <path-to-extra-json-file>
```

This CUDA path tracer currently only support `.obj` scene and support reading material from corresponding `.mtl` file. `.json` file is used to specify the camera.

## Build

CMake is used to build this project.

C++20 is needed.

GPU that supports CUDA is needed.

## Used Thirdparty

* [glad](https://github.com/Dav1dde/glad)
* [glfw](https://github.com/glfw/glfw)
* [glm](https://github.com/g-truc/glm)
* [Dear ImGui](https://github.com/ocornut/imgui)
* [stb image](https://github.com/nothings/stb)
* [nlohmann json](https://github.com/nlohmann/json)
* [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)

## Details

The main 2 purposes for me to write this project are to

1. implement Linear BVH building using CUDA
2. try to implement a very naive component system (just for fun)

In this naive component system, a block linked list is used to store components for each component type, so it may be quick to loop over entities with a single specific component type but slow to loop over components of a specific entity. There is no need to derive some base class, each class can be a component type directly.

