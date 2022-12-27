#pragma once

#include "common.cuh"
#include "../basic/frame.cuh"

namespace kernel {

struct PhongBsdf {
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    float s_norm;

    CU_DEVICE bool IsDelta() const { return false; }

    CU_DEVICE BsdfSample Sample(const glm::vec3 &wo, float rand1, const glm::vec2 &rand2) const {
        auto diffuse_weight = Luminance(diffuse);
        auto specular_weight = Luminance(specular);
        auto diffuse_pdf = diffuse_weight / (diffuse_weight + specular_weight);
        auto specular_pdf = 1.0f - diffuse_pdf;

        BsdfSample samp {};
        if (rand1 < diffuse_pdf) {
            samp.wi = CosineHemisphereSample(rand2);
            samp.wi.z = copysignf(samp.wi.z, wo.z);
            samp.lobe = { BsdfLobe::Type::eDiffuse, BsdfLobe::Dir::eReflection };
        } else {
            auto phi = rand2.x * k2Pi;
            auto sin_phi = sin(phi);
            auto cos_phi = cos(phi);
            auto cos_theta = pow(1.0f - rand2.y, 1.0f / (1.0f + shininess));
            auto sin_theta = sqrt(1.0f - cos_theta * cos_theta);
            glm::vec3 wi(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);

            Frame frame(glm::vec3(-wo.x, -wo.y, wo.z));
            samp.wi = frame.ToWorld(wi);
            if (samp.wi.z * wo.x <= 0.0f) {
                samp.pdf = 0.0f;
                return samp;
            }
            samp.lobe = { BsdfLobe::Type::eGlossy, BsdfLobe::Dir::eReflection };
        }
        samp.pdf = Pdf(wo, samp.wi);
        samp.weight = Eval(wo, samp.wi) / samp.pdf;

        return samp;
    }

    CU_DEVICE float Pdf(const glm::vec3 &wo, const glm::vec3 &wi) const {
        if (wo.z * wi.z <= 0.0f) {
            return 1.0f;
        }

        auto diffuse_weight = Luminance(diffuse);
        auto specular_weight = Luminance(specular);
        auto diffuse_pdf = diffuse_weight / (diffuse_weight + specular_weight);
        auto specular_pdf = 1.0f - diffuse_pdf;
        auto r = glm::vec3(-wo.x, -wo.y, wo.z);

        return diffuse_pdf * abs(wi.z) * kInvPi + specular_pdf * pow(glm::dot(r, wi), shininess) * s_norm;
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
