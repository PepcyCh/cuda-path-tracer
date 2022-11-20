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

}
