#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "cuda_helpers/buffer.hpp"

class CameraComponent {
public:
    CuBuffer *Buffer() const { return buffer_.get(); }

    void Update();

    void ShowUi();

    void BuildBuffer();

    glm::vec3 pos = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 look_at = glm::vec3(0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float fov = 45.0f;

    uint32_t film_width = 1280;
    uint32_t film_height = 720;

private:
    std::unique_ptr<CuBuffer> buffer_;
};
