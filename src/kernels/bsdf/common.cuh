#pragma once

#include "../basic/prelude.cuh"

namespace kernel {

struct BsdfLobe {
    enum struct Type : uint16_t {
        eNone,
        eDiffuse,
        eGlossy,
        eSpecular,
    } type;
    enum struct Dir : uint16_t {
        eNone,
        eReflection,
        eTransmission,
    } dir;
};

struct BsdfSample {
    glm::vec3 wi = glm::vec3(0.0f);
    float pdf = 0.0f;
    glm::vec3 weight = glm::vec3(0.0f);
    BsdfLobe lobe = { BsdfLobe::Type::eNone, BsdfLobe::Dir::eNone };
};

inline CU_DEVICE glm::vec3 CosineHemisphereSample(const glm::vec2 &rand) {
    auto phi = rand.x * k2Pi;
    auto sin_phi = sin(phi);
    auto cos_phi = cos(phi);
    auto sin_theta = sqrt(rand.y);
    auto cos_theta = sqrt(1.0f - rand.y);
    glm::vec3 wi(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
    return wi;
}

inline CU_DEVICE glm::vec3 Reflect(const glm::vec3 &i, const glm::vec3 &n) {
    return 2.0f * glm::dot(i, n) * n - i;
}
inline CU_DEVICE glm::vec3 Refract(const glm::vec3 &i, const glm::vec3 &n, float ior) {
    auto cos_i = glm::dot(i, n);
    auto eta = cos_i >= 0.0f ? 1.0f / ior : ior;
    auto cos_t_sqr = 1.0f - (1.0f - cos_i * cos_i) * eta * eta;
    if (cos_t_sqr < 0.0f) {
        return glm::vec3(0.0f);
    }
    auto cos_t = sqrt(cos_t_sqr);
    auto n_scale = eta * abs(cos_i) - cos_t;
    return (cos_i >= 0.0f ? n_scale : -n_scale) * n - eta * i;
}

inline CU_DEVICE glm::vec3 ReflectHalf(const glm::vec3 &i, const glm::vec3 &o) {
    auto h = glm::normalize(i + o);
    return h.z >= 0.0f ? h : -h;
}
inline CU_DEVICE glm::vec3 RefractHalf(const glm::vec3 &i, const glm::vec3 &o, float ior) {
    auto eta = i.z >= 0.0f ? 1.0f / ior : ior;
    auto h = glm::normalize(eta * i + o);
    return h.z >= 0.0f ? h : -h;
}

inline CU_DEVICE float Pow2(float x) {
    return x * x;
}
inline CU_DEVICE float Pow5(float x) {
    float x2 = Pow2(x);
    return x2 * x2 * x;
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

}
