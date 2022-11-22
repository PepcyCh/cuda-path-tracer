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
        Vertex vertex {};
        switch (type) {
            case Type::eTriMesh:
                vertex = reinterpret_cast<const TriMesh *>(ptr)->SampleVertex(rand);
                break;
        }
        VertexTransformPdf(vertex, transform);
        vertex.position = transform * glm::vec4(vertex.position, 1.0f);
        vertex.normal = glm::normalize(transform_it * glm::vec4(vertex.normal, 0.0f));
        return vertex;
    }
};

}
