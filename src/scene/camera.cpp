#include "camera.hpp"

#include <imgui.h>

#include "pathtracer/pathtracer.hpp"
#include "kernels/camera/camera.cuh"

void CameraComponent::Update() {
    // TODO - camera & key
}

void CameraComponent::ShowUi() {
    bool changed = false;
    changed |= ImGui::DragFloat3("pos", &pos.x, 0.05f);
    changed |= ImGui::DragFloat3("look at", &look_at.x, 0.05f);
    changed |= ImGui::DragFloat3("up", &up.x, 0.05f);
    changed |= ImGui::DragFloat("fov", &fov, 0.5f);

    if (changed) {
        BuildBuffer();

        GetGlobalScene().ForEach<PathTracer>([](PathTracer &path_tracer) {
            path_tracer.ResetAccumelation();
        });
    }
}

void CameraComponent::BuildBuffer() {
    auto frame_z = glm::normalize(pos - look_at);
    auto frame_x = glm::normalize(glm::cross(up, frame_z));
    auto frame_y = glm::normalize(glm::cross(frame_z, frame_x));

    float scale = std::tan(glm::radians(fov * 0.5f)) * 2.0f;
    glm::vec3 x_dir = glm::vec3(scale, 0.0f, 0.0f);
    glm::vec3 y_dir = glm::vec3(0.0f, scale, 0.0f);

    kernel::PinholeCamera data {
        .pos = pos,
        .frame_x = frame_x,
        .frame_y = frame_y,
        .frame_z = frame_z,
        .x_dir = x_dir,
        .y_dir = y_dir,
    };
    if (!buffer_) {
        buffer_ = std::make_unique<CuBuffer>(sizeof(data), &data);
    } else {
        buffer_->SetData(&data, sizeof(data));
    }
}
