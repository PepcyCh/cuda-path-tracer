#pragma once

#include "directional.cuh"
#include "geometry.cuh"

namespace kernel {
    
struct Light {
    enum struct Type {
        eDirectional,
        eGeometry,
    } type;
    void *ptr;

    CU_DEVICE bool IsDelta() const {
        switch (type) {
            case Type::eDirectional:
                return reinterpret_cast<const DirLight *>(ptr)->IsDelta();
            case Type::eGeometry:
                return reinterpret_cast<const GeometryLight *>(ptr)->IsDelta();
        }
    }

    CU_DEVICE LightSample Sample(const glm::vec3 &pos, const glm::vec2 &rand) const {
        switch (type) {
            case Type::eDirectional:
                return reinterpret_cast<const DirLight *>(ptr)->Sample(pos, rand);
            case Type::eGeometry:
                return reinterpret_cast<const GeometryLight *>(ptr)->Sample(pos, rand);
        }
    }
};

struct LightSampler {
    Light *lights;
    uint32_t num_lights;
    float pdf;

    CU_DEVICE Light Sample(const glm::vec3 &pos, float rand, float &pdf) const {
        auto index = glm::min(static_cast<uint32_t>(rand * num_lights), num_lights - 1);
        pdf = this->pdf;
        return lights[index];
    }

    CU_DEVICE float Pdf(const glm::vec3 &pos, const Light &light) const {
        return pdf;
    }
};

}
