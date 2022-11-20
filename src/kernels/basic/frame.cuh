#pragma once

#include "prelude.cuh"

namespace kernel {

struct Frame {
    glm::vec3 origin;
    glm::vec3 x;
    glm::vec3 y;
    glm::vec3 z;

    CU_DEVICE Frame(const glm::vec3 &normal, const glm::vec3 &origin = glm::vec3(0.0f)) : origin(origin) {
        z = normal;
        auto sign = normal.z > 0.0f ? 1.0f : -1.0f;
        auto a = -1.0f / (sign + normal.z);
        auto b = normal.x * normal.y * a;
        x = glm::vec3(1.0f + sign * normal.x * normal.x * a, sign * b, -sign * normal.x);
        y = glm::vec3(b, sign + normal.y * normal.y * a, -normal.y);
    }

    CU_DEVICE Frame(const glm::vec3 &x, const glm::vec3 &y, const glm::vec3 &z,
        const glm::vec3 &origin = glm::vec3(0.0f)) : origin(origin), x(x), y(y), z(z) {}

    CU_DEVICE glm::vec3 ToLocal(const glm::vec3 &p, float w = 0.0f) {
        auto pp = p - origin * w;
        return glm::vec3(glm::dot(x, pp), glm::dot(y, pp), glm::dot(z, pp));
    }

    CU_DEVICE glm::vec3 ToWorld(const glm::vec3 &p, float w = 0.0f) {
        return x * p.x + y * p.y + z * p.z + w * origin;
    }
};

}
