#pragma once

#include "film.hpp"
#include "cuda_helpers/buffer.hpp"
#include "scene/core.hpp"

class PathTracer {
public:
    PathTracer(Scene &scene, Film &film);

    void BuildBuffers();

    void ResetAccumelation();

    void Update();

    void ShowUi();

    void SetCaptureName(std::string_view capture_name) { capture_name_ = capture_name; }
    void SetMaxDepth(int max_depth) { max_depth_ = max_depth; }

private:
    void BuildAccel();
    void BuildInstancesAndLights();

    Scene &scene_;
    Film &film_;

    uint32_t num_captured_frames_ = 0;
    std::string capture_name_;

    int max_depth_ = -1;
    uint32_t curr_spp_ = 0;
    int display_channel_ = 0;

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
