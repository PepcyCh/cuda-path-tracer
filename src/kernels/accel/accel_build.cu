#include "accel_build.cuh"

#include <memory>

#include <thrust/sort.h>

#include "cuda_helpers/buffer.hpp"

namespace kernel {

namespace {

constexpr float kMortonCodeResolution = 1024.0f;

constexpr uint32_t kThreads = 32;

CU_DEVICE uint32_t MortonCode3(uint32_t x) {
    x = (x ^ (x << 16)) & 0xff0000ff;
	x = (x ^ (x << 8)) & 0x0300f00f;
	x = (x ^ (x << 4)) & 0x030c30c3;
	x = (x ^ (x << 2)) & 0x09249249;
    return x;
}

CU_GLOBAL void CalcMortonCode(uint64_t *codes, const Bbox *bboxes, Bbox merged_bbox, uint32_t num_primitives) {
    uint32_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= num_primitives) {
        return;
    }

    auto center = (bboxes[index].pmin + bboxes[index].pmax) * 0.5f;
    auto scale = merged_bbox.pmax - merged_bbox.pmin;
    auto p = (center - merged_bbox.pmin) / scale;

    auto x = MortonCode3(fmin(p.x * kMortonCodeResolution, kMortonCodeResolution - 1));
    auto y = MortonCode3(fmin(p.y * kMortonCodeResolution, kMortonCodeResolution - 1));
    auto z = MortonCode3(fmin(p.z * kMortonCodeResolution, kMortonCodeResolution - 1));
    uint64_t morton_code = (x << 2) | (y << 1) | z;

    codes[index] = (morton_code << 32) | index;
}

CU_GLOBAL void FillLeafNodes(AccelNode *nodes, const uint64_t *codes, uint32_t num_primitives) {
    uint32_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= num_primitives) {
        return;
    }

    nodes[index].lc_or_id = codes[index];
    nodes[index].rc = ~0u;
}

CU_DEVICE uint32_t Lcp(uint64_t a, uint64_t b) {
    return __clzll(a ^ b);
}

CU_DEVICE glm::uvec2 FindNodeRange(uint32_t index, const uint64_t *codes, uint32_t num_primitives) {
    if (index == 0) {
        return glm::uvec2(0, num_primitives - 1);
    }

    auto code = codes[index];
    auto l_lcp = Lcp(code, codes[index - 1]);
    auto r_lcp = Lcp(code, codes[index + 1]);
    
    auto d = l_lcp > r_lcp ? -1 : 1;
    auto min_lcp = glm::min(l_lcp, r_lcp);
    uint32_t step = 1;
    uint32_t j_lcp;
    do {
        step <<= 1;
        int j = index + d * step;
        j_lcp = 0;
        if (j >= 0 && j < num_primitives) {
            j_lcp = Lcp(code, codes[j]);
        }
    } while (j_lcp > min_lcp);

    auto l = step >> 1;
    auto r = glm::min(step, d == -1 ? index : num_primitives - 1 - index);
    while (l < r) {
        auto mid = l + ((r - l) >> 1) + 1;
        auto j = index + d * mid;
        auto j_lcp = Lcp(code, codes[j]);
        if (j_lcp < min_lcp) {
            r = mid - 1;
        } else {
            l = mid;
        }
    }

    r = index + d * l;
    l = index;
    return l <= r ? glm::uvec2(l, r) : glm::uvec2(r, l);
}

CU_DEVICE uint32_t FindNodeLeftChild(uint32_t index, glm::uvec2 range, const uint64_t *codes, uint32_t num_primitives) {
    auto l_code = codes[range.x];
    auto r_code = codes[range.y];
    auto node_lcp = Lcp(l_code, r_code);

    auto l = range.x;
    auto r = range.y;
    while (l < r) {
        auto mid = l + ((r - l) >> 1);
        auto m_lcp = Lcp(l_code, codes[mid]);
        if (m_lcp > node_lcp) {
            l = mid + 1;
        } else {
            r = mid;
        }
    }

    return l - 1;
}

CU_GLOBAL void BuildInternalNodes(AccelNode *nodes, uint32_t *parents, const uint64_t *codes, uint32_t num_primitives) {
    uint32_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= num_primitives - 1) {
        return;
    }

    auto range = FindNodeRange(index, codes, num_primitives);
    auto lc = FindNodeLeftChild(index, range, codes, num_primitives);
    auto rc = lc + 1;

    if (lc == range.x) {
        lc += num_primitives - 1;
    }
    if (rc == range.y) {
        rc += num_primitives - 1;
    }

    nodes[index].lc_or_id = lc;
    nodes[index].rc = rc;
    parents[lc] = index;
    parents[rc] = index;
}

CU_GLOBAL void InitInternalNodesBbox(Bbox *bboxes, Bbox merged_bbox, uint32_t num_primitives) {
    uint32_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= num_primitives - 1) {
        return;
    }

    bboxes[index].pmin = merged_bbox.pmax;
    bboxes[index].pmax = merged_bbox.pmin;
}

CU_DEVICE float atomicMinFloat(float *addr, float value) {
    return !signbit(value) ? __int_as_float(atomicMin(reinterpret_cast<int *>(addr), __float_as_int(value))) :
        __uint_as_float(atomicMax(reinterpret_cast<uint32_t *>(addr), __float_as_uint(value)));
}

CU_DEVICE float atomicMaxFloat(float *addr, float value) {
    return !signbit(value) ? __int_as_float(atomicMax(reinterpret_cast<int *>(addr), __float_as_int(value))) :
        __uint_as_float(atomicMin(reinterpret_cast<uint32_t *>(addr), __float_as_uint(value)));
}

CU_GLOBAL void CalcInternalNodesBbox(const uint32_t *parents, Bbox *bboxes, uint32_t num_primitives) {
    uint32_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= num_primitives) {
        return;
    }

    uint32_t u = num_primitives - 1 + index;
    uint32_t pa = parents[u];
    auto bbox = bboxes[u];
    while (u != 0) {
        atomicMinFloat(&bboxes[pa].pmin.x, bbox.pmin.x);
        atomicMinFloat(&bboxes[pa].pmin.y, bbox.pmin.y);
        atomicMinFloat(&bboxes[pa].pmin.z, bbox.pmin.z);
        atomicMaxFloat(&bboxes[pa].pmax.x, bbox.pmax.x);
        atomicMaxFloat(&bboxes[pa].pmax.y, bbox.pmax.y);
        atomicMaxFloat(&bboxes[pa].pmax.z, bbox.pmax.z);

        u = pa;
        pa = parents[u];
    }
}

}

void BuildAccel(AccelNode *nodes, Bbox *bboxes, Bbox merged_bbox, uint32_t num_primitives) {
    auto num_internal_nodes = num_primitives - 1;

    auto morton_codes_buffer = std::make_unique<CuBuffer>(sizeof(uint64_t) * num_primitives);
    auto morton_codes = morton_codes_buffer->TypedGpuData<uint64_t>();
    auto bboxes_leaf = bboxes + num_internal_nodes;
    CalcMortonCode<<<(num_primitives + kThreads - 1) / kThreads, kThreads>>>(
        morton_codes, bboxes_leaf, merged_bbox, num_primitives);

    thrust::sort_by_key(thrust::device, morton_codes, morton_codes + num_primitives, bboxes_leaf);

    auto nodes_leaf = nodes + num_internal_nodes;
    FillLeafNodes<<<(num_primitives + kThreads - 1) / kThreads, kThreads>>>(nodes_leaf, morton_codes, num_primitives);

    auto parents_buffer = std::make_unique<CuBuffer>(sizeof(uint32_t) * (num_primitives + num_internal_nodes));
    auto parents = parents_buffer->TypedGpuData<uint32_t>();
    BuildInternalNodes<<<(num_internal_nodes + kThreads - 1) / kThreads, kThreads>>>(
        nodes, parents, morton_codes, num_primitives);

    InitInternalNodesBbox<<<(num_internal_nodes + kThreads - 1) / kThreads, kThreads>>>(
        bboxes, merged_bbox, num_primitives);
    CalcInternalNodesBbox<<<(num_primitives + kThreads - 1) / kThreads, kThreads>>>(parents, bboxes, num_primitives);
}

}
