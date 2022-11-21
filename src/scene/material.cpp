#include "material.hpp"

#include <imgui.h>

#include "pathtracer/pathtracer.hpp"
#include "kernels/material/material.cuh"

void Material::SetChanged() {
    BuildBuffer();
}

void Material::BuildBuffer() {
    kernel::MtlMaterial data {
        {
            {
                .value = emission,
                .texture = emission_map ? emission_map->Object() : 0,
            },
        },
        {
            .value = diffuse,
            .texture = diffuse_map ? diffuse_map->Object() : 0,
        },
        {
            .value = specular,
            .texture = specular_map ? specular_map->Object() : 0,
        },
        shininess,
    };
    if (!buffer_) {
        buffer_ = std::make_unique<CuBuffer>(sizeof(data), &data);
    } else {
        buffer_->SetData(&data, sizeof(data));
    }

    ptr_ = kernel::TaggedPointer {
        .tag = static_cast<uint32_t>(kernel::Material::Type::eMtl),
        .ptr = buffer_->GpuData(),
    };
}

bool Material::IsEmissive() const {
    return glm::dot(emission, emission) > 0.0f;
}

void MaterialComponent::ShowUi() {
    bool changed = false;
    changed |= ImGui::DragFloat3("emission", &material_->emission.x, 0.0f);
    changed |= ImGui::DragFloat3("diffuse", &material_->diffuse.x, 0.01f, 0.0f, 1.0f);
    changed |= ImGui::DragFloat3("specular", &material_->specular.x, 0.01f, 0.0f, 1.0f);
    changed |= ImGui::DragFloat("shiniess", &material_->shininess, 0.1f, 0.0f, 1000.0f);
    changed |= ImGui::DragFloat("ior", &material_->ior, 0.001f, 1.0f, 2.0f);

    if (changed) {
        material_->SetChanged();

        GetGlobalScene().ForEach<PathTracer>([](PathTracer &path_tracer) {
            path_tracer.ResetAccumelation();
        });
    }
}
