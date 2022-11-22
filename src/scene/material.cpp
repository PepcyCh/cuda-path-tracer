#include "material.hpp"

#include <imgui.h>

#include "pathtracer/pathtracer.hpp"
#include "kernels/material/material.cuh"

namespace {

float Sqr(float x) {
    return x * x;
}

}

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
        {
            .value = transmittance,
            .texture = 0,
        },
        shininess,
        ior,
        opacity,
        static_cast<kernel::Bsdf::Type>(bsdf_type),
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

void Material::ShowUi() {
    bool changed = false;
    changed |= ImGui::DragFloat3("emission", &emission.x, 0.0f);
    changed |= ImGui::DragFloat3("diffuse", &diffuse.x, 0.01f, 0.0f, 1.0f);
    changed |= ImGui::DragFloat3("specular", &specular.x, 0.01f, 0.0f, 1.0f);
    changed |= ImGui::DragFloat3("transmit", &transmittance.x, 0.01f, 0.0f, 1.0f);

    auto roughness = sqrt(2.0f / (2.0f + shininess));
    if (ImGui::DragFloat("shiniess", &shininess, 0.1f, 0.0f, 1000.0f)) {
        roughness = sqrt(2.0f / (2.0f + shininess));
        changed = true;
    }
    if (ImGui::DragFloat("roughness", &roughness, 0.001f, 0.001f, 1.0f)) {
        shininess = 2.0f / Sqr(roughness) - 2.0f;
        changed = true;
    }

    changed |= ImGui::DragFloat("ior", &ior, 0.001f, 1.0f, 2.0f);
    changed |= ImGui::DragFloat("opacity", &opacity, 0.001f, 0.0f, 1.0f);
    ImGui::Text("emission map: %s", emission_map ? "true" : "false");
    ImGui::Text("diffuse map: %s", diffuse_map ? "true" : "false");
    ImGui::Text("specular map: %s", specular_map ? "true" : "false");
    const char *bsdf_type_name[] = {
        "Lambert",
        "Blinn-Phong",
        "Microfacet",
    };
    changed |= ImGui::Combo("bsdf", &bsdf_type, bsdf_type_name,
        sizeof(bsdf_type_name) / sizeof(bsdf_type_name[0]));

    if (changed) {
        SetChanged();

        GetGlobalScene().ForEach<PathTracer>([](PathTracer &path_tracer) {
            path_tracer.ResetAccumelation();
        });
    }
}

void MaterialComponent::ShowUi() {
    material_->ShowUi();
}
