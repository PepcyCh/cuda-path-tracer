#pragma once

#include "../scene/scene.cuh"

namespace kernel {

struct PathTracer {
    struct Params {
        Scene scene;
        glm::vec4 *output;
        uint32_t screen_width;
        uint32_t screen_height;
        uint32_t spp;
        uint32_t max_depth;
    };

    static void Render(const Params &params);
};

}
