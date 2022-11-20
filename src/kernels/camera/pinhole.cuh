#pragma once

#include "../basic/ray.cuh"
#include "../basic/frame.cuh"

namespace kernel {

struct PinholeCamera {
    glm::vec3 pos;
    glm::vec3 frame_x;
    glm::vec3 frame_y;
    glm::vec3 frame_z;
    glm::vec3 x_dir;
    glm::vec3 y_dir;

    CU_DEVICE Ray SampleRay(float aspect, const glm::vec2 &position_rand, const glm::vec2 &apreture_rand) const {
        auto dir = glm::normalize((x_dir * aspect * (position_rand.x - 0.5f)) + (y_dir * (position_rand.y - 0.5f))
            + glm::vec3(0.0f, 0.0f, -1.0f));
        Frame frame(frame_x, frame_y, frame_z);
        dir = frame.ToWorld(dir);
        return Ray(pos, dir);
    }
};

}
