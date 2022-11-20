#pragma once

#include "lambert.cuh"

namespace kernel {

struct Bsdf {
    static constexpr uint32_t kMaxSize = 48;

    enum struct Type {
        eLambert,
    } type;
    glm::vec3 emission;
    uint8_t data[kMaxSize];

    CU_DEVICE bool IsDelta() const {
        switch (type) {
            case Type::eLambert:
                return reinterpret_cast<const LambertBsdf *>(data)->IsDelta();
        }
    }

    CU_DEVICE BsdfSample Sample(const glm::vec3 &wo, float rand1, const glm::vec2 &rand2) const {
        switch (type) {
            case Type::eLambert:
                return reinterpret_cast<const LambertBsdf *>(data)->Sample(wo, rand1, rand2);
        }
    }

    CU_DEVICE float Pdf(const glm::vec3 &wo, const glm::vec3 &wi) const {
        switch (type) {
            case Type::eLambert:
                return reinterpret_cast<const LambertBsdf *>(data)->Pdf(wo, wi);
        }
    }

    CU_DEVICE glm::vec3 Eval(const glm::vec3 &wo, const glm::vec3 &wi) const {
        switch (type) {
            case Type::eLambert:
                return reinterpret_cast<const LambertBsdf *>(data)->Eval(wo, wi);
        }
    }
};

}
