#pragma once

#include "common.cuh"

namespace kernel {

struct DirLight {
    glm::vec3 dir;
    glm::vec3 emission;

    CU_DEVICE bool IsDelta() const { return true; }

    CU_DEVICE LightSample Sample(const glm::vec3 &pos, const glm::vec2 &rand) const {
        LightSample samp {};
        samp.dir = -dir;
        samp.pdf = 1.0f;
        samp.weight = emission;
        samp.dist = FLT_MAX;
        return samp;
    }
};

}
