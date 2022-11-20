#pragma once

#include "prelude.cuh"

namespace kernel {

struct Ray {
    static constexpr float kDefaultTMin = 1e-5f;
    static constexpr float kDefaultTMax = std::numeric_limits<float>::max();
    static constexpr float kShadowRayEps = 0.001f;

    glm::vec3 origin;
    float tmin = kDefaultTMin;
    glm::vec3 direction;
    float tmax = kDefaultTMax;

    CU_DEVICE Ray(glm::vec3 origin, glm::vec3 direction) : origin(origin), direction(direction) {}

    CU_DEVICE glm::vec3 At(float t) const { return origin + t * direction; }
};

}
