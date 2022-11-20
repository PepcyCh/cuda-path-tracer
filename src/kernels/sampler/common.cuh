#pragma once

#include "../basic/prelude.cuh"

namespace kernel {

struct SamplerState {
    enum struct Type {
        eRandom,
    } type;
    uint32_t state;

    static CU_DEVICE SamplerState Create(uint32_t x, uint32_t y, Type type = Type::eRandom);

    CU_DEVICE float Next1D();
    CU_DEVICE glm::vec2 Next2D();
};

}
