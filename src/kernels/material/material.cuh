#pragma once

#include "lambert.cuh"

namespace kernel {

struct Material {
    enum struct Type {
        eLambert,
    } type;
    MaterialCommon *ptr;

    CU_DEVICE Bsdf GetBsdf(const glm::vec2 &uv) const {
        Bsdf bsdf {};
        switch (type) {
            case Type::eLambert:
                bsdf = reinterpret_cast<const LambertMaterial *>(ptr)->GetBsdf(uv);
                break;
        }
        bsdf.emission = ptr->emission.At(uv);
        return bsdf;
    }
};

}
