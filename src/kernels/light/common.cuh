#pragma once

#include "../basic/prelude.cuh"

namespace kernel {

struct LightSample {
    glm::vec3 dir = glm::vec3(0.0f);
    float pdf = 0.0f;
    glm::vec3 weight = glm::vec3(0.0f);
    float dist = 0.0f;
};

}
