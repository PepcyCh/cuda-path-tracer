cmake_minimum_required(VERSION 3.18)
project(cuda-pathtracer LANGUAGES C CXX CUDA)


find_package(CUDAToolkit REQUIRED)

add_subdirectory(thirdparty/glfw)

add_library(glad thirdparty/glad/src/glad.c)
target_include_directories(glad PUBLIC thirdparty/glad/include)

add_subdirectory(thirdparty/tinyobjloader)

add_subdirectory(thirdparty/glm)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE thirdparty/stb)

add_library(json INTERFACE)
target_include_directories(json INTERFACE thirdparty/json/include)

file(GLOB IMGUI_SOURCES thirdparty/imgui/*.cpp)
add_library(imgui ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC thirdparty/imgui)
target_link_libraries(imgui PUBLIC glfw)


file(GLOB_RECURSE HEADERS src/*.hpp)
file(GLOB_RECURSE SOURCES src/*.cpp src/*.cu)
add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE CUDA::cudart CUDA::cuda_driver glfw glad tinyobjloader glm stb json imgui)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--extended-lambda>)
