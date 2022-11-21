#pragma once

#include "../basic/frame.cuh"
#include "../scene/scene.cuh"
#include "../sampler/sampler.cuh"

namespace kernel {

inline CU_DEVICE float PowerHeuristic(float p0, float p1) {
    p0 *= p0;
    p1 *= p1;
    return p0 / (p0 + p1);
}

inline CU_DEVICE float BalanceHeuristic(float p0, float p1) {
    return p0 / (p0 + p1);
}

inline CU_DEVICE float PathToSolidAngleJacobian(const glm::vec3 &p_prev, const glm::vec3 &p, const glm::vec3 &n) {
    auto vec = p_prev - p;
    auto dist_sqr = dot(vec, vec);
    auto dir = vec * glm::inversesqrt(dist_sqr);
    auto cos_theta = dot(n, dir);
    return cos_theta > 0.0f ? dist_sqr / cos_theta : 0.0f;
}

inline CU_DEVICE float SolidAngleToPathJacobian(const glm::vec3 &p_prev, const glm::vec3 &p, const glm::vec3 &n) {
    auto vec = p_prev - p;
    auto dist_sqr = dot(vec, vec);
    auto dir = vec * glm::inversesqrt(dist_sqr);
    return fmax(glm::dot(n, dir), 0.0f) / dist_sqr;
}
    
}
