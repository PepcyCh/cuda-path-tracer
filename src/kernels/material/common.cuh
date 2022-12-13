#pragma once

#include <texture_types.h>

#include "../bsdf/bsdf.cuh"

namespace kernel {

#ifdef __CUDACC__
#define READ_TEX2D(result, texture, u, v) auto result = tex2D<float4>(texture, u, v)
#else
#define READ_TEX2D(result, texture, u, v) glm::vec4 result(0.0f)
#endif

struct MaterialValue {
    glm::vec3 value;
    cudaTextureObject_t texture;

    CU_DEVICE glm::vec3 At(const glm::vec2 &uv) const {
        auto res = value;
        if (texture) {
            READ_TEX2D(tex_value, texture, uv.x, 1.0f - uv.y);
            res = glm::vec3(tex_value.x, tex_value.y, tex_value.z);
        }
        return res;
    }
};

struct MaterialCommon {
    MaterialValue emission;
};
    
}
