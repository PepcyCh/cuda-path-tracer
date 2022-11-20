#pragma once

#include "trimesh.cuh"

namespace kernel {

struct Geometry {
    enum struct Type {
        eTriMesh,
    } type;
    void *ptr;

    CU_DEVICE bool Intersect(const Ray &ray, uint32_t primitive_id, float &t, glm::vec2 &attribs) const {
        switch (type) {
            case Type::eTriMesh:
                return reinterpret_cast<const TriMesh *>(ptr)->Intersect(ray, primitive_id, t, attribs);
        }
    }

    CU_DEVICE Vertex GetVertex(uint32_t primitive_id, const glm::vec2 &attribs) const {
        switch (type) {
            case Type::eTriMesh:
                return reinterpret_cast<const TriMesh *>(ptr)->GetVertex(primitive_id, attribs);
        }
    }

    CU_DEVICE Vertex SampleVertex(const glm::mat4 &transform, const glm::mat4 &transform_it,
        const glm::vec2 &rand) const {
        Vertex vert {};
        switch (type) {
            case Type::eTriMesh:
                vert = reinterpret_cast<const TriMesh *>(ptr)->SampleVertex(rand);
                break;
        }
        // TODO - transform pdf
        vert.position = transform * glm::vec4(vert.position, 1.0f);
        vert.normal = glm::normalize(transform_it * glm::vec4(vert.normal, 0.0f));
        return vert;
    }
};

}
