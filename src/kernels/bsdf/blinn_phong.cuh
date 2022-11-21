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
        // if (rand1 < diffuse_pdf) {
        //     samp.wi = CosineHemisphereSample(rand2);
        //     auto d = diffuse * abs(samp.wi.z) * kInvPi;

        //     auto h = glm::normalize(samp.wi + wo);
        //     auto s = specular * pow(abs(h.z), shininess) * (2.0f + shininess) * kInv2Pi;
        //     auto half_pdf = pow(abs(h.z), shininess) * (2.0f + shininess) * kInv2Pi;
        //     samp.pdf = diffuse_pdf * abs(samp.wi.z) + specular_pdf * half_pdf / (4.0f * glm::dot(wo, h));
        //     samp.weight = (d + s) / samp.pdf;
        //     samp.lobe = { BsdfLobe::Type::eDiffuse, BsdfLobe::Dir::eReflection };
        // } else {
        //     auto phi = rand2.x * k2Pi;
        //     auto sin_phi = sin(phi);
        //     auto cos_phi = cos(phi);
        //     auto cos_theta = pow(1.0f - rand2.y, 1.0f / (2.0f + shininess));
        //     auto sin_theta = sqrt(1.0f - cos_theta * cos_theta);
        //     glm::vec3 h(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
        //     auto half_pdf = pow(abs(h.z), shininess) * (2.0f + shininess) * kInv2Pi;
        //     samp.wi = Reflect(wo, h);
        //     samp.pdf = diffuse_pdf * abs(samp.wi.z) + specular_pdf * half_pdf / (4.0f * cos_theta);
        //     samp.weight = (diffuse * abs(samp.wi.z) * kInvPi + specular * half_pdf / (4.0f * glm::dot(wo, h)))
        //         / samp.pdf;
        //     samp.lobe = { BsdfLobe::Type::eGlossy, BsdfLobe::Dir::eReflection };
        // }

        samp.wi = CosineHemisphereSample(rand2);
        samp.pdf = abs(samp.wi.z);
        samp.weight = Eval(wo, samp.wi) / samp.pdf;
        samp.lobe = { BsdfLobe::Type::eDiffuse, BsdfLobe::Dir::eReflection };

        return samp;
    }

    CU_DEVICE float Pdf(const glm::vec3 &wo, const glm::vec3 &wi) const {
        if (wo.z * wi.z <= 0.0f) {
            return 1.0f;
        }

        // auto diffuse_weight = Luminance(diffuse);
        // auto specular_weight = Luminance(specular);
        // auto diffuse_pdf = diffuse_weight / (diffuse_weight + specular_weight);
        // auto specular_pdf = 1.0f - diffuse_pdf;
        // auto h = glm::normalize(wi + wo);
        // auto s = specular * pow(abs(h.z), shininess) * (2.0f + shininess) * kInv2Pi;
        // auto half_pdf = pow(abs(h.z), shininess) * (2.0f + shininess) * kInv2Pi;
        // return diffuse_pdf * abs(wi.z) + specular_pdf * half_pdf / (4.0f * glm::dot(wo, h));
        return abs(wi.z);
    }

    CU_DEVICE glm::vec3 Eval(const glm::vec3 &wo, const glm::vec3 &wi) const {
        if (wo.z * wi.z <= 0.0f) {
            return glm::vec3(0.0f);
        }
        auto h = glm::normalize(wi + wo);
        auto d = diffuse * abs(wi.z) * kInvPi;
        auto s = specular * pow(abs(h.z), shininess) * (2.0f + shininess) * kInv2Pi;
        return d + s;
    }
};

}
