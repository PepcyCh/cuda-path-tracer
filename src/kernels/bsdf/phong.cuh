#pragma once

#include "common.cuh"

namespace kernel {

struct PhongBsdf {
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    float s_norm;

    CU_DEVICE bool IsDelta() const { return false; }

    CU_DEVICE BsdfSample Sample(const glm::vec3 &wo, float rand1, const glm::vec2 &rand2) const {
        BsdfSample samp {};
        samp.wi = CosineHemisphereSample(rand2);
        samp.wi.z = copysignf(samp.wi.z, wo.z);
        samp.lobe = { BsdfLobe::Type::eGlossy, BsdfLobe::Dir::eReflection };
        samp.pdf = Pdf(wo, samp.wi);
        samp.weight = Eval(wo, samp.wi) / samp.pdf;

        return samp;
    }

    CU_DEVICE float Pdf(const glm::vec3 &wo, const glm::vec3 &wi) const {
        if (wo.z * wi.z <= 0.0f) {
            return 1.0f;
        }

        return abs(wi.z) * kInvPi;
    }

    CU_DEVICE glm::vec3 Eval(const glm::vec3 &wo, const glm::vec3 &wi) const {
        if (wo.z * wi.z <= 0.0f) {
            return glm::vec3(0.0f);
        }
        auto r = glm::vec3(-wo.x, -wo.y, wo.z);
        auto d = diffuse * abs(wi.z) * kInvPi;
        auto s = specular * pow(glm::dot(r, wi), shininess) * s_norm;
        return d + s;
    }
};

}
