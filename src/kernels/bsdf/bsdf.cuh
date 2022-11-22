#pragma once

#include "lambert.cuh"
#include "blinn_phong.cuh"
#include "microfacet.cuh"

namespace kernel {

struct Bsdf {
    static constexpr uint32_t kMaxSize = 48;

    enum struct Type {
        eLambert,
        eBlinnPhong,
        eMicrofacet,
    } type;
    glm::vec3 emission;
    uint8_t data[kMaxSize];

    CU_DEVICE bool IsDelta() const {
        switch (type) {
            case Type::eLambert:
                return reinterpret_cast<const LambertBsdf *>(data)->IsDelta();
            case Type::eBlinnPhong:
                return reinterpret_cast<const BlinnPhongBsdf *>(data)->IsDelta();
            case Type::eMicrofacet:
                return reinterpret_cast<const MicrofacetBsdf *>(data)->IsDelta();
        }
    }

    CU_DEVICE BsdfSample Sample(const glm::vec3 &wo, float rand1, const glm::vec2 &rand2) const {
        switch (type) {
            case Type::eLambert:
                return reinterpret_cast<const LambertBsdf *>(data)->Sample(wo, rand1, rand2);
            case Type::eBlinnPhong:
                return reinterpret_cast<const BlinnPhongBsdf *>(data)->Sample(wo, rand1, rand2);
            case Type::eMicrofacet:
                return reinterpret_cast<const MicrofacetBsdf *>(data)->Sample(wo, rand1, rand2);
        }
    }

    CU_DEVICE float Pdf(const glm::vec3 &wo, const glm::vec3 &wi) const {
        switch (type) {
            case Type::eLambert:
                return reinterpret_cast<const LambertBsdf *>(data)->Pdf(wo, wi);
            case Type::eBlinnPhong:
                return reinterpret_cast<const BlinnPhongBsdf *>(data)->Pdf(wo, wi);
            case Type::eMicrofacet:
                return reinterpret_cast<const MicrofacetBsdf *>(data)->Pdf(wo, wi);
        }
    }

    CU_DEVICE glm::vec3 Eval(const glm::vec3 &wo, const glm::vec3 &wi) const {
        switch (type) {
            case Type::eLambert:
                return reinterpret_cast<const LambertBsdf *>(data)->Eval(wo, wi);
            case Type::eBlinnPhong:
                return reinterpret_cast<const BlinnPhongBsdf *>(data)->Eval(wo, wi);
            case Type::eMicrofacet:
                return reinterpret_cast<const MicrofacetBsdf *>(data)->Eval(wo, wi);
        }
    }
};

}
