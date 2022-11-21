#pragma once

#include "common.cuh"

namespace kernel {

namespace {

CU_DEVICE uint32_t RandInitTea(uint32_t x, uint32_t y) {
    uint32_t v0 = x;
    uint32_t v1 = y;
    uint32_t s0 = 0;
    for (uint32_t n = 0; n < 16; n++) {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

CU_DEVICE uint32_t RandPcg(uint32_t &state) {
    auto prev = state;
    state = state * 747796405u + 2891336453u;
    auto word = ((prev >> ((prev >> 28u) + 4u)) ^ prev) * 277803737u;
    return (word >> 22u) ^ word;
}

}

struct RandomSampler {
    static CU_DEVICE SamplerState Creare(uint32_t x, uint32_t y) {
        return SamplerState {
            SamplerState::Type::eRandom,
            RandInitTea(x, y),
        };
    }

    static CU_DEVICE float Next1D(SamplerState &state) {
        auto rnd = RandPcg(state.state);
        return rnd / 4294967296.0f;
    }

    static CU_DEVICE glm::vec2 Next2D(SamplerState &state) {
        return glm::vec2(Next1D(state), Next1D(state));
    }
};

}
