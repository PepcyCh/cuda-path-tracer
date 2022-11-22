#pragma once

#include "../basic/ray.cuh"
#include "../basic/frame.cuh"

namespace kernel {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
    float pdf;
};

inline CU_DEVICE void VertexTransformPdf(Vertex &vertex, const glm::mat4 &transform) {
    auto frame_local = Frame(vertex.normal);
    auto t = glm::mat3(transform) * frame_local.x;
    auto b = glm::mat3(transform) * frame_local.y;
    vertex.pdf /= glm::length(glm::cross(t, b));
}

}
