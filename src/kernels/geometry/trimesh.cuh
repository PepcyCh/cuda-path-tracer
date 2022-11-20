#pragma once

#include "common.cuh"

namespace kernel {

struct TriMesh {
    glm::vec3 *positions;
    glm::vec3 *normals;
    glm::vec2 *texcoords;
    uint32_t *indices;
    uint32_t num_triangles;

    CU_DEVICE bool Intersect(const Ray &ray, uint32_t primitive_id, float &t, glm::vec2 &attribs) const {
        auto i0 = indices[primitive_id * 3];
        auto i1 = indices[primitive_id * 3 + 1];
        auto i2 = indices[primitive_id * 3 + 2];
        auto p0 = positions[i0];
        auto p1 = positions[i1];
        auto p2 = positions[i2];
        auto e1 = p1 - p0;
        auto e2 = p2 - p0;
        auto q = glm::cross(ray.direction, e2);
        auto det = glm::dot(e1, q);
        if (det != 0.0f) {
            det = 1.0f / det;
            auto s = ray.origin - p0;
            auto v = glm::dot(s, q) * det;
            if (v >= 0.0f) {
                auto r = glm::cross(s, e1);
                auto w = glm::dot(ray.direction, r) * det;
                auto u = 1.0f - v - w;
                if (w >= 0.0f && u >= 0.0f) {
                    t = glm::dot(e2, r) * det;
                    if (t > ray.tmin && t < ray.tmax) {
                        attribs = glm::vec2(v, w);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    CU_DEVICE Vertex GetVertex(uint32_t primitive_id, const glm::vec2 &attribs) const {
        auto i0 = indices[primitive_id * 3];
        auto i1 = indices[primitive_id * 3 + 1];
        auto i2 = indices[primitive_id * 3 + 2];

        Vertex vert {};

        auto pos0 = positions[i0];
        auto pos1 = positions[i1];
        auto pos2 = positions[i2];
        auto e1 = pos1 - pos0;
        auto e2 = pos2 - pos0;
        auto area = glm::length(glm::cross(e1, e2)) * 0.5f;
        vert.pdf = 1.0f / area / num_triangles;
        vert.position = pos0 + attribs.x * (pos1 - pos0) + attribs.y * (pos2 - pos0);

        auto norm0 = normals[i0];
        auto norm1 = normals[i1];
        auto norm2 = normals[i2];
        vert.normal = glm::normalize(norm0 + attribs.x * (norm1 - norm0) + attribs.y * (norm2 - norm0));

        auto tc0 = texcoords[i0];
        auto tc1 = texcoords[i1];
        auto tc2 = texcoords[i2];
        vert.texcoord = tc0 + attribs.x * (tc1 - tc0) + attribs.y * (tc2 - tc0);

        return vert;
    }

    CU_DEVICE Vertex SampleVertex(const glm::vec2 &rand) const {
        auto primitive_id = glm::min(static_cast<uint32_t>(rand.x * num_triangles), num_triangles - 1);
        float u_sqrt = sqrt(rand.x * num_triangles - primitive_id);
        float u = 1.0f - u_sqrt;
        float v = (1.0f - rand.y) * u_sqrt;
        return GetVertex(primitive_id, glm::vec2(u, v));
    }
};

}
