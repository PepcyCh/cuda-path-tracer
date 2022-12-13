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
        if (bsdf_type != Bsdf::Type::eMicrofacet && opacity == 0.0f) {
            Bsdf bsdf { Bsdf::Type::eGlass };
            auto data = reinterpret_cast<GlassBsdf *>(bsdf.data);
            // data->reflectance = specular.At(uv);
            data->transmittance = transmittance.At(uv);
            data->reflectance = data->transmittance;
            data->ior = ior;
            return bsdf;
        }

        Bsdf bsdf { bsdf_type };
        switch (bsdf_type) {
            case Bsdf::Type::eLambert: {
                auto data = reinterpret_cast<LambertBsdf *>(bsdf.data);
                data->color = diffuse.At(uv);
                break;
            }
            case Bsdf::Type::ePhong: {
                auto data = reinterpret_cast<PhongBsdf *>(bsdf.data);
                data->diffuse = diffuse.At(uv);
                data->specular = specular.At(uv);
                data->shininess = shininess;
                data->s_norm = (shininess + 1) * kInv2Pi;
                break;
            }
            case Bsdf::Type::eBlinnPhong: {
                auto data = reinterpret_cast<BlinnPhongBsdf *>(bsdf.data);
                data->diffuse = diffuse.At(uv);
                data->specular = specular.At(uv);
                data->shininess = shininess;
                data->s_norm = (shininess + 2) / (4 * kPi * (2 - pow(2.0f, -shininess / 2)));
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
            default:
                break;
        }
        return bsdf;
    }
};

}
