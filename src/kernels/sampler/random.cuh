#pragma once

#include "common.cuh"

namespace kernel {

namespace {

CU_DEVICE uint32_t HashCombine(uint32_t x, uint32_t y) {
    x ^= y + 0x9e3779b9 + (x << 6) + (x >> 2);
    return x;
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
            HashCombine(x, y),
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
