#pragma once

#include "film.hpp"
#include "cuda_helpers/buffer.hpp"
#include "scene/core.hpp"

class PathTracer {
public:
    PathTracer(Scene &scene);

    void BuildBuffers();

    void Update();

    void Render(Film *film);

private:
    void BuildAccel();
    void BuildInstancesAndLights();

    void ResetAccumelation();

    Scene &scene_;

    int max_depth_ = -1;
    uint32_t curr_spp_ = 0;

    uint32_t last_width_ = 0;
    uint32_t last_height_ = 0;

    std::unique_ptr<CuBuffer> accel_buffer_;
    std::unique_ptr<CuBuffer> accel_nodes_buffer_;
    std::unique_ptr<CuBuffer> accel_bboxes_buffer_;
    std::unique_ptr<CuBuffer> accel_instances_buffer_;

    std::unique_ptr<CuBuffer> instances_buffer_;
    std::unique_ptr<CuBuffer> lights_buffer_;
    uint32_t num_lights_ = 0;
    std::vector<std::unique_ptr<CuBuffer>> geo_light_buffers_;

    CuBuffer *camera_buffer_ = nullptr;
};
