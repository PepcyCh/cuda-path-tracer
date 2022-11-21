#pragma once

#include "common.cuh"

namespace kernel {

struct LambertBsdf {
    glm::vec3 color;

    CU_DEVICE bool IsDelta() const { return false; }

    CU_DEVICE BsdfSample Sample(const glm::vec3 &wo, float rand1, const glm::vec2 &rand2) const {
        auto wi = CosineHemisphereSample(rand2);
        return BsdfSample { wi, abs(wi.z) * kInvPi, color, { BsdfLobe::Type::eDiffuse, BsdfLobe::Dir::eReflection} };
    }

    CU_DEVICE float Pdf(const glm::vec3 &wo, const glm::vec3 &wi) const {
        return wo.z * wi.z >= 0.0f ? abs(wi.z) * kInvPi : 1.0f;
    }

    CU_DEVICE glm::vec3 Eval(const glm::vec3 &wo, const glm::vec3 &wi) const {
        return wo.z * wi.z >= 0.0f ? color * abs(wi.z) * kInvPi : glm::vec3(0.0f);
    }
};

}
