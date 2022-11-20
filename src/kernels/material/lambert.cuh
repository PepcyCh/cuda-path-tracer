#pragma once

#include "common.cuh"

namespace kernel {

struct LambertMaterial : MaterialCommon {
    MaterialValue color;

    CU_DEVICE Bsdf GetBsdf(const glm::vec2 &uv) const {
        Bsdf bsdf { Bsdf::Type::eLambert };
        auto data = reinterpret_cast<LambertBsdf *>(bsdf.data);
        data->color = color.At(uv);
        return bsdf;
    }
};

}
