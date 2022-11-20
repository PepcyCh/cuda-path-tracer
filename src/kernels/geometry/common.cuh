#pragma once

#include "../basic/ray.cuh"

namespace kernel {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
    float pdf;
};

}
