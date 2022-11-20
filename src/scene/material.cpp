#include "material.hpp"

#include <imgui.h>

#include "kernels/material/material.cuh"

void Material::BuildBuffer() {
    kernel::LambertMaterial data {
        {
            {
                .value = emission,
                .texture = emission_map ? emission_map->Object() : 0,
            },
        },
        {
            .value = color,
            .texture = color_map ? color_map->Object() : 0,
        }
    };
    if (!buffer_) {
        buffer_ = std::make_unique<CuBuffer>(sizeof(data), &data);
    } else {
        buffer_->SetData(&data, sizeof(data));
    }
}

bool Material::IsEmissive() const {
    return glm::dot(emission, emission) > 0.0f;
}

void MaterialComponent::ShowUi() {
    ImGui::DragFloat3("diffuse", &material_->color.x, 0.0f);
    ImGui::DragFloat3("emission", &material_->emission.x, 0.0f);
}
