#pragma once

#include "instance.cuh"
#include "../camera/camera.cuh"

namespace kernel {

struct Scene {
    Camera camera;
    LightSampler light_sampler;
    Instance *instances;
    AccelTop *accel;
};

}
