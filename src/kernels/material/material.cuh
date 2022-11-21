#pragma once

#include "lambert.cuh"
#include "mtl.cuh"

namespace kernel {

struct Material {
    enum struct Type {
        eLambert,
        eMtl,
    } type;
    MaterialCommon *ptr;

    CU_DEVICE Bsdf GetBsdf(const glm::vec2 &uv) const {
        Bsdf bsdf {};
        switch (type) {
            case Type::eLambert:
                bsdf = reinterpret_cast<const LambertMaterial *>(ptr)->GetBsdf(uv);
                break;
            case Type::eMtl:
                bsdf = reinterpret_cast<const MtlMaterial *>(ptr)->GetBsdf(uv);
                break;
        }
        bsdf.emission = ptr->emission.At(uv);
        return bsdf;
    }
};

}
