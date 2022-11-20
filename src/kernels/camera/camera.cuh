#pragma once

#include "pinhole.cuh"

namespace kernel {

struct Camera {
    enum struct Type {
        ePinhole,
    } type;
    void *ptr;

    CU_DEVICE Ray SampleRay(float aspect, const glm::vec2 &position_rand, const glm::vec2 &apreture_rand) const {
        switch (type) {
            case Type::ePinhole:
                return reinterpret_cast<const PinholeCamera *>(ptr)->SampleRay(aspect, position_rand, apreture_rand);
        }
    }
};

}
