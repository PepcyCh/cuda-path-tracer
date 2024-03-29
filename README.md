# CUDA Path Tracer

A basic path tracer written in C++20 and CUDA with a naive component system.

（Also project for Zhejiang University 2022-2023 Autumn-Winter Advanced Computer Graphics master course）

Usage:

```
cuda-pathtracer <path-to-obj-file> <path-to-extra-file> [OPTIONS]

OPTIONS
  --ui            0 or 1, whether show UI (default 1)
  --max-depth     max ray tracing depth (default -1, no limit)
  --max-spp       max ray tracing spp when ui is 0 (default -1, no limit)
  --output | -o   output .exr name (default 'capture')
  --bsdf-type     which BSDF to use (default 'blinn-phong')
```

This CUDA path tracer currently only support `.obj` scene and support reading material from corresponding `.mtl` file. Another file (`.json` or `.xml`) is used to specify the camera and some other info.

## Build

CMake is used to build this project.

C++20 is needed.

GPU and driver that support CUDA (CUDA 11.x) and OpenGL 3.3 are needed.

## Used Thirdparty

* [glad](https://github.com/Dav1dde/glad)
* [glfw](https://github.com/glfw/glfw)
* [glm](https://github.com/g-truc/glm)
* [Dear ImGui](https://github.com/ocornut/imgui)
* [stb image](https://github.com/nothings/stb)
* [nlohmann json](https://github.com/nlohmann/json)
* [tinyxml2](https://github.com/leethomason/tinyxml2)
* [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)
* [tinyexr](https://github.com/syoyo/tinyexr)

## Details

The 2 main purposes for me to write this project are to

1. implement Linear BVH building using CUDA
2. try to implement a very naive component system (just for fun)

In this naive component system, a block linked list is used to store components for each component type, so it may be quick to loop over entities with a single specific component type but slow to loop over components of a specific entity. There is no need to derive some base class, each class can be a component type directly.

![](./pic/readme.jpg)
