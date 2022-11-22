#pragma once

#include "common.cuh"

namespace kernel {

namespace {

inline CU_DEVICE float GgxNdf(float ndoth, float a2) {
    return a2 * kInvPi / Pow2(ndoth * ndoth * (a2 - 1.0f) + 1.0f);
}

inline CU_DEVICE float SmithHeightCorrelatedVisible(float ndotv, float ndotl, float a2) {
    auto v = ndotl * sqrt(a2 + (1.0f - a2) * ndotv * ndotv);
    auto l = ndotv * sqrt(a2 + (1.0f - a2) * ndotl * ndotl);
    return 0.5f / (v + l);
}

inline CU_DEVICE float Fresnel(float ior, const glm::vec3 &i, const glm::vec3 &n) {
    auto eta = glm::dot(i, n) < 0.0f ? ior : 1.0f / ior;
    auto refract = Refract(i, n, ior);
    if (refract != glm::vec3(0.0f)) {
        auto idotn = abs(glm::dot(i, n));
        auto tdotn = abs(glm::dot(refract, n));

        auto rs = Pow2((idotn - eta * tdotn) / (idotn + eta * tdotn));
        auto rp = Pow2((tdotn - eta * idotn) / (tdotn + eta * idotn));
        return 0.5f * (rs + rp);
    } else {
        return 1.0f;
    }
}

inline CU_DEVICE float GgxNdfSampleCos2(float a2, float rand) {
    return (1.0f - rand) / (1.0f - rand * (1.0f - a2));
}

}

struct MicrofacetBsdf {
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 transmittance;
    float ior;
    float roughness;
    float opacity;

    CU_DEVICE bool IsDelta() const { return false; }

    CU_DEVICE BsdfSample Sample(const glm::vec3 &wo, float rand1, const glm::vec2 &rand2) const {
        auto fr_macro = Fresnel(ior, wo, glm::vec3(0.0f, 0.0f, 1.0f));
        auto specular_weight = Luminance(specular) * fr_macro;
        auto diffuse_weight = Luminance(diffuse) * (1.0f - fr_macro) * opacity;
        auto transmit_weight = Luminance(transmittance) * (1.0f - fr_macro) * (1.0f - opacity);
        auto inv = 1.0f / (specular_weight + diffuse_weight + transmit_weight);
        specular_weight *= inv;
        diffuse_weight *= inv;
        transmit_weight *= inv;

        BsdfSample samp {};
        if (rand1 < diffuse_weight) {
            samp.wi = CosineHemisphereSample(rand2);
            samp.lobe = { BsdfLobe::Type::eDiffuse, BsdfLobe::Dir::eReflection };
        } else {
            auto a2 = roughness * roughness;
            auto phi = rand2.x * k2Pi;
            auto sin_phi = sin(phi);
            auto cos_phi = cos(phi);
            auto cos_theta2 = GgxNdfSampleCos2(a2, rand2.y);
            auto cos_theta = sqrt(cos_theta2);
            auto sin_theta = sqrt(1.0f - cos_theta2);
            glm::vec3 h(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);

            if (rand1 < diffuse_weight + specular_weight) {
                samp.wi = Reflect(wo, h);
                samp.lobe = { BsdfLobe::Type::eGlossy, BsdfLobe::Dir::eReflection };
            } else {
                samp.wi = Refract(wo, h, ior);
                if (samp.wi == glm::vec3(0.0f)) {
                    samp.pdf = 0.0f;
                    samp.lobe = {};
                    return samp;
                }
                samp.lobe = { BsdfLobe::Type::eGlossy, BsdfLobe::Dir::eTransmission };
            }
        }

        samp.pdf = Pdf(wo, samp.wi);
        samp.weight = Eval(wo, samp.wi) / samp.pdf;

        return samp;
    }

    CU_DEVICE float Pdf(const glm::vec3 &wo, const glm::vec3 &wi) const {
        auto fr_macro = Fresnel(ior, wo, glm::vec3(0.0f, 0.0f, 1.0f));
        auto specular_weight = Luminance(specular) * fr_macro;
        auto diffuse_weight = Luminance(diffuse) * (1.0f - fr_macro) * opacity;
        auto transmit_weight = Luminance(transmittance) * (1.0f - fr_macro) * (1.0f - opacity);
        auto inv = 1.0f / (specular_weight + diffuse_weight + transmit_weight);
        specular_weight *= inv;
        diffuse_weight *= inv;
        transmit_weight *= inv;

        auto a2 = roughness * roughness;

        if (wo.z * wi.z <= 0.0f) {
            auto h = RefractHalf(wi, wo, ior);

            auto ndf = GgxNdf(h.z, a2);
            auto eta = wo.z >= 0.0f ? 1.0f / ior : ior;
            auto denom = Pow2(eta * glm::dot(wo, h) + glm::dot(wi, h));
            auto num = abs(glm::dot(wi, h));
            return transmit_weight * ndf * num / denom;
        } else {
            auto h = ReflectHalf(wi, wo);

            auto ndf = GgxNdf(h.z, a2);
            auto d = abs(wi.z) * kInvPi;
            auto s = ndf / (4.0f * abs(glm::dot(wi, h)));
            return diffuse_weight * d + specular_weight * s;
        }
    }

    CU_DEVICE glm::vec3 Eval(const glm::vec3 &wo, const glm::vec3 &wi) const {
        if (wo.z * wi.z <= 0.0f) {
            auto h = RefractHalf(wi, wo, ior);
            auto fr = Fresnel(ior, wo, h);
            auto ft = 1.0f - fr;
            auto eta = wo.z >= 0.0f ? 1.0f / ior : ior;

            auto a2 = roughness * roughness;
            auto ndf = GgxNdf(h.z, a2);
            auto vis = SmithHeightCorrelatedVisible(abs(wo.z), abs(wi.z), a2);
            auto denom = Pow2(eta * glm::dot(wo, h) + glm::dot(wi, h));
            auto num = 4.0f * abs(glm::dot(wo, h)) * abs(glm::dot(wi, h)) * eta * eta;
            auto t = transmittance * ft * (1.0f - opacity) * ndf * vis * num / denom * abs(wi.z);

            return t;
        } else {
            auto h = ReflectHalf(wi, wo);
            auto fr = Fresnel(ior, wo, h);
            auto ft = 1.0f - fr;

            auto a2 = roughness * roughness;
            auto ndf = GgxNdf(h.z, a2);
            auto vis = SmithHeightCorrelatedVisible(abs(wo.z), abs(wi.z), a2);
            auto s = specular * fr * ndf * vis * abs(wi.z);

            auto d = diffuse * ft * opacity * abs(wi.z) * kInvPi;

            return d + s;
        }
    }
};

}
