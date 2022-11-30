#pragma once

#include "common.cuh"

namespace kernel {

struct GlassBsdf {
    glm::vec3 reflectance;
    glm::vec3 transmittance;
    float ior;

    CU_DEVICE bool IsDelta() const { return true; }

    CU_DEVICE BsdfSample Sample(const glm::vec3 &wo, float rand1, const glm::vec2 &rand2) const {
        auto fr = Fresnel(ior, wo, glm::vec3(0.0f, 0.0f, 1.0f));
        auto ft = 1.0f - fr;
        auto reflect_weight = fr * Luminance(reflectance);
        auto transmit_weight = ft * Luminance(transmittance);
        auto reflect_pdf = reflect_weight / (reflect_weight + transmit_weight);
        auto transmit_pdf = 1.0f - reflect_pdf;

        BsdfSample samp {};
        if (rand1 < reflect_pdf) {
            samp.wi = Reflect(wo, glm::vec3(0.0f, 0.0f, 1.0f));
            samp.pdf = reflect_pdf;
            samp.weight = fr * reflectance / reflect_pdf;
            samp.lobe = { BsdfLobe::Type::eSpecular, BsdfLobe::Dir::eReflection };
        } else {
            samp.wi = Refract(wo, glm::vec3(0.0f, 0.0f, 1.0f), ior);
            if (samp.wi == glm::vec3(0.0f)) {
                samp.pdf = 0.0f;
            } else {
                auto eta = wo.z >= 0.0f ? 1.0f / ior : ior;
                samp.pdf = transmit_pdf;
                samp.weight = eta * eta * ft * transmittance / transmit_pdf;
                samp.lobe = { BsdfLobe::Type::eSpecular, BsdfLobe::Dir::eTransmission };
            }
        }

        return samp;
    }

    CU_DEVICE float Pdf(const glm::vec3 &wo, const glm::vec3 &wi) const {
        auto fr = Fresnel(ior, wo, glm::vec3(0.0f, 0.0f, 1.0f));
        auto ft = 1.0f - fr;
        auto reflect_weight = fr * Luminance(reflectance);
        auto transmit_weight = ft * Luminance(transmittance);
        auto reflect_pdf = reflect_weight / (reflect_weight + transmit_weight);
        auto transmit_pdf = 1.0f - reflect_pdf;

        return wo.z * wi.z >= 0.0f ? reflect_pdf : transmit_pdf;
    }

    CU_DEVICE glm::vec3 Eval(const glm::vec3 &wo, const glm::vec3 &wi) const {
        auto fr = Fresnel(ior, wo, glm::vec3(0.0f, 0.0f, 1.0f));
        auto ft = 1.0f - fr;
        if (wo.z * wi.z <= 0.0f) {
            auto eta = wo.z >= 0.0f ? 1.0f / ior : ior;
            return eta * eta * ft * transmittance;
        } else {
            return fr * reflectance;
        }
    }
};

}
