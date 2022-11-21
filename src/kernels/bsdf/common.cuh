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
    return 2.0f * n - i;
}

}
