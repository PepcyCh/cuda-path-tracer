#pragma once

#include "common.cuh"

namespace kernel {

struct BlinnPhongBsdf {
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;

    CU_DEVICE bool IsDelta() const { return false; }

    CU_DEVICE BsdfSample Sample(const glm::vec3 &wo, float rand1, const glm::vec2 &rand2) const {
        auto diffuse_weight = Luminance(diffuse);
        auto specular_weight = Luminance(specular);
        auto diffuse_pdf = diffuse_weight / (diffuse_weight + specular_weight);
        auto specular_pdf = 1.0f - diffuse_pdf;

        BsdfSample samp {};
        float d = 0.0f;
        float s = 0.0f;
        glm::vec3 h(0.0f);
        if (rand1 < diffuse_pdf) {
            samp.wi = CosineHemisphereSample(rand2);
            samp.wi.z = copysignf(samp.wi.z, wo.z);
            samp.lobe = { BsdfLobe::Type::eDiffuse, BsdfLobe::Dir::eReflection };

            d = abs(samp.wi.z) * kInvPi;
            h = ReflectHalf(samp.wi, wo);
            s = pow(h.z, shininess);
        } else {
            auto phi = rand2.x * k2Pi;
            auto sin_phi = sin(phi);
            auto cos_phi = cos(phi);
            auto cos_theta = pow(1.0f - rand2.y, 1.0f / (2.0f + shininess));
            auto sin_theta = sqrt(1.0f - cos_theta * cos_theta);
            h = glm::vec3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
            samp.wi = Reflect(wo, h);
            samp.lobe = { BsdfLobe::Type::eGlossy, BsdfLobe::Dir::eReflection };

            s = pow(h.z, shininess);
            d = abs(samp.wi.z) * kInvPi;
        }
        auto half_pdf = s * (2.0f + shininess) * kInv2Pi;
        samp.pdf = diffuse_pdf * d + specular_pdf * half_pdf / (4.0f * abs(glm::dot(samp.wi, h)));
        samp.weight = (diffuse * d + specular * s) / samp.pdf;

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
        auto h = ReflectHalf(wi, wo);
        auto half_pdf = pow(h.z, shininess) * (2.0f + shininess) * kInv2Pi;
        return diffuse_pdf * abs(wi.z) * kInvPi + specular_pdf * half_pdf / (4.0f * abs(glm::dot(wi, h)));
    }

    CU_DEVICE glm::vec3 Eval(const glm::vec3 &wo, const glm::vec3 &wi) const {
        if (wo.z * wi.z <= 0.0f) {
            return glm::vec3(0.0f);
        }
        auto h = ReflectHalf(wi, wo);
        auto d = diffuse * abs(wi.z) * kInvPi;
        auto s = specular * pow(h.z, shininess);
        return d + s;
    }
};

}
