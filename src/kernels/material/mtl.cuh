#pragma once

#include "common.cuh"

namespace kernel {

struct MtlMaterial : MaterialCommon {
    MaterialValue diffuse;
    MaterialValue specular;
    float shininess;

    CU_DEVICE Bsdf GetBsdf(const glm::vec2 &uv) const {
        Bsdf bsdf { Bsdf::Type::eBlinnPhong };
        auto data = reinterpret_cast<BlinnPhongBsdf *>(bsdf.data);
        data->diffuse = diffuse.At(uv);
        data->specular = specular.At(uv);
        data->shininess = shininess;
        return bsdf;
    }
};

}
