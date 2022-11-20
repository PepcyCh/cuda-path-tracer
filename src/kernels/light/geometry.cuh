#pragma once

#include "common.cuh"
#include "../geometry/geometry.cuh"
#include "../material/material.cuh"

namespace kernel {

struct GeometryLight {
    Geometry geometry;
    Material material;
    glm::mat4 transform;
    glm::mat4 transform_it;

    CU_DEVICE bool IsDelta() const { return false; }

    CU_DEVICE LightSample Sample(const glm::vec3 &pos, const glm::vec2 &rand) const {
        auto vert = geometry.SampleVertex(transform, transform_it, rand);
        auto vec = vert.position - pos;
        auto dist_sqr = glm::dot(vec, vec);
        auto dist = sqrt(dist_sqr);
        auto dir = vec / dist;
        LightSample samp {};
        samp.dir = dir;
        samp.dist = dist;
        float cos_theta = dot(-dir, vert.normal);
        samp.pdf = cos_theta > 0.0f ? vert.pdf * dist_sqr / cos_theta : 0.0f;
        samp.weight = material.ptr->emission.At(vert.texcoord) / samp.pdf;
        return samp;
    }
};

}
