#pragma once

#ifdef __CUDACC__
#define CU_GLOBAL __global__
#define CU_DEVICE __device__
#define CU_HOST __host__
#define CU_DEVICE_HOST __device__ __host__
#else
#define CU_GLOBAL
#define CU_DEVICE
#define CU_HOST
#define CU_DEVICE_HOST
#endif

#include <glm/glm.hpp>

namespace kernel {

constexpr float kPi = 3.14159265359f;
constexpr float k2Pi = 2.0f * kPi;
constexpr float kInvPi = 1.0f / kPi;
constexpr float kInv2Pi = 1.0f / k2Pi;

inline CU_DEVICE float Luminance(const glm::vec3 &color) {
    return color.r * 0.299 + color.g * 0.587 + color.b * 0.114f;
}

struct TaggedPointer {
    uint32_t tag;
    void *ptr;
};

}
