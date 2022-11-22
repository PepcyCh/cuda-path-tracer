#pragma once

#include "common.cuh"

namespace kernel {

struct MtlMaterial : MaterialCommon {
    MaterialValue diffuse;
    MaterialValue specular;
    MaterialValue transmittance;
    float shininess;
    float ior;
    float opacity;
    Bsdf::Type bsdf_type;

    CU_DEVICE Bsdf GetBsdf(const glm::vec2 &uv) const {
        Bsdf bsdf { bsdf_type };
        switch (bsdf_type) {
            case Bsdf::Type::eLambert: {
                auto data = reinterpret_cast<LambertBsdf *>(bsdf.data);
                data->color = diffuse.At(uv);
                break;
            }
            case Bsdf::Type::eBlinnPhong: {
                auto data = reinterpret_cast<BlinnPhongBsdf *>(bsdf.data);
                data->diffuse = diffuse.At(uv);
                data->specular = specular.At(uv);
                data->shininess = shininess;
                break;
            }
            case Bsdf::Type::eMicrofacet: {
                auto data = reinterpret_cast<MicrofacetBsdf *>(bsdf.data);
                data->diffuse = diffuse.At(uv);
                data->specular = specular.At(uv);
                data->transmittance = transmittance.At(uv);
                data->roughness = sqrt(2.0f / (2.0f + shininess));
                data->ior = ior;
                data->opacity = opacity;
                break;
            }
        }
        return bsdf;
    }
};

}
