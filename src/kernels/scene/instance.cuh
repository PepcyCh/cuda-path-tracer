#pragma once

#include "../accel/accel.cuh"
#include "../material/material.cuh"
#include "../light/light.cuh"

namespace kernel {

struct ShadingSurface {
    Vertex vertex;
    Bsdf bsdf;
};

struct Instance {
    Geometry geometry;
    Material material;
    Light light;

    CU_DEVICE ShadingSurface GetShadingSurface(const AccelHitInfo &hit_info) const {
        auto transform_it = glm::transpose(hit_info.transform_inv);
        auto vertex = geometry.GetVertex(hit_info.primitive_id, hit_info.attribs);
        VertexTransformPdf(vertex, hit_info.transform);
        vertex.position = hit_info.transform * glm::vec4(vertex.position, 1.0f);
        vertex.normal = glm::normalize(transform_it * glm::vec4(vertex.normal, 0.0f));
        return ShadingSurface { vertex, material.GetBsdf(vertex.texcoord )};
    }
};

}
