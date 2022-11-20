#pragma once

#include "random.cuh"

namespace kernel {

CU_DEVICE SamplerState SamplerState::Create(uint32_t x, uint32_t y, Type type) {
    switch (type) {
        case Type::eRandom:
            return RandomSampler::Creare(x, y);
    }
}

CU_DEVICE float SamplerState::Next1D() {
    switch (type) {
        case Type::eRandom:
            return RandomSampler::Next1D(*this);
    }
}

CU_DEVICE glm::vec2 SamplerState::Next2D() {
    switch (type) {
        case Type::eRandom:
            return RandomSampler::Next2D(*this);
    }
}

}
