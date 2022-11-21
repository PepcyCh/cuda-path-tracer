#include "pathtracer.hpp"

#include <imgui.h>

#include "scene/mesh.hpp"
#include "scene/material.hpp"
#include "scene/camera.hpp"
#include "kernels/accel/accel_build.cuh"
#include "kernels/integrator/path.cuh"

PathTracer::PathTracer(Scene &scene, Film &film) : scene_(scene), film_(film) {}

void PathTracer::BuildBuffers() {
    BuildAccel();

    scene_.ForEach<CameraComponent>([this](CameraComponent &camera) {
        camera.BuildBuffer();
        camera_buffer_ = camera.Buffer();
    });

    BuildInstancesAndLights();
}

void PathTracer::Update() {
    if (!camera_buffer_ || num_lights_ == 0) {
        return;
    }

    if (film_.Width() != last_width_ || film_.Height() != last_height_) {
        ResetAccumelation();
        last_width_ = film_.Width();
        last_height_ = film_.Height();
    }

    ++curr_spp_;
    kernel::PathTracer::Params params {
        .scene = {
            .camera = {
                .type = kernel::Camera::Type::ePinhole,
                .ptr = camera_buffer_->GpuData(),
            },
            .light_sampler = {
                .lights = lights_buffer_->TypedGpuData<kernel::Light>(),
                .num_lights = num_lights_,
                .pdf = num_lights_ != 0 ? 1.0f / num_lights_ : 0.0f,
            },
            .instances = instances_buffer_->TypedGpuData<kernel::Instance>(),
            .accel = accel_buffer_->TypedGpuData<kernel::AccelTop>(),
        },
        .output = film_.CudaMap(),
        .screen_width = film_.Width(),
        .screen_height = film_.Height(),
        .spp = curr_spp_,
        .max_depth = static_cast<uint32_t>(max_depth_),
    };
    kernel::PathTracer::Render(params);
    film_.CudaUnmap();
}

void PathTracer::ShowUi() {
    bool changed = false;
    changed |= ImGui::DragInt("max depth", &max_depth_, 1, -1, 16);
    ImGui::Text("accumelated spp: %u", curr_spp_);
    changed |= ImGui::Button("reset accumeltaion");

    if (changed) {
        ResetAccumelation();
    }
}

void PathTracer::ResetAccumelation() {
    curr_spp_ = 0;
}

void PathTracer::BuildAccel() {
    uint32_t num_instances = scene_.Count<MeshComponent>();
    uint32_t num_accel_nodes = num_instances * 2 - 1;

    std::vector<kernel::Bbox> accel_leaf_bboxes;
    accel_leaf_bboxes.reserve(num_instances);
    std::vector<kernel::AccelTop::Instance> accel_instances;
    accel_instances.reserve(num_instances);
    scene_.ForEach<MeshComponent, const MaterialComponent>(
        [&accel_leaf_bboxes, &accel_instances](SceneObject &object, MeshComponent &mesh, const MaterialComponent &) {
            mesh.GetMesh()->BuildAccel();

            auto bbox = mesh.GetMesh()->Bbox();
            auto trans = object.GetTransform();
            bbox = bbox.TransformBy(trans);
            accel_leaf_bboxes.push_back(kernel::Bbox {
                .pmin = glm::vec4(bbox.pmin, 1.0f),
                .pmax = glm::vec4(bbox.pmax, 1.0f),
            });
            accel_instances.push_back(kernel::AccelTop::Instance {
                .accel = mesh.GetMesh()->AccelBuffer()->TypedGpuData<kernel::AccelBottom>(),
                .transform = trans,
                .transform_inv = glm::inverse(trans),
            });
        }
    );
    num_instances = accel_instances.size();
    num_accel_nodes = num_instances * 2 - 1;

    std::vector<kernel::Bbox> accel_bboxes(num_accel_nodes);
    std::copy(accel_leaf_bboxes.begin(), accel_leaf_bboxes.end(), accel_bboxes.begin() + num_instances - 1);

    kernel::Bbox merged_bbox {
        .pmin = glm::vec4(glm::vec3(std::numeric_limits<float>::max()), 1.0f),
        .pmax = glm::vec4(glm::vec3(std::numeric_limits<float>::lowest()), 1.0f),
    };
    for (size_t i = 0; i < num_instances; i++) {
        merged_bbox.pmin = glm::min(merged_bbox.pmin, accel_bboxes[num_instances - 1 + i].pmin);
        merged_bbox.pmax = glm::max(merged_bbox.pmax, accel_bboxes[num_instances - 1 + i].pmax);
    }
    
    auto bbox_buffer_size = sizeof(kernel::Bbox) * num_accel_nodes;
    if (!accel_bboxes_buffer_ || accel_bboxes_buffer_->Size() < bbox_buffer_size) {
        accel_bboxes_buffer_ = std::make_unique<CuBuffer>(bbox_buffer_size, accel_bboxes.data());
    } else {
        accel_bboxes_buffer_->SetData(accel_bboxes.data(), bbox_buffer_size);
    }
    auto instance_buffer_size = sizeof(kernel::AccelTop::Instance) * num_accel_nodes;
    if (!accel_instances_buffer_ || accel_instances_buffer_->Size() < instance_buffer_size) {
        accel_instances_buffer_ = std::make_unique<CuBuffer>(instance_buffer_size, accel_instances.data());
    } else {
        accel_instances_buffer_->SetData(accel_instances.data(), instance_buffer_size);
    }

    auto node_buffer_size = sizeof(kernel::AccelNode) * num_accel_nodes;
    if (!accel_nodes_buffer_ || accel_nodes_buffer_->Size() < node_buffer_size) {
        accel_nodes_buffer_ = std::make_unique<CuBuffer>(node_buffer_size);
    }

    kernel::BuildAccel(
        accel_nodes_buffer_->TypedGpuData<kernel::AccelNode>(),
        accel_bboxes_buffer_->TypedGpuData<kernel::Bbox>(),
        merged_bbox, num_instances
    );

    kernel::AccelTop accel {
        .nodes = accel_nodes_buffer_->TypedGpuData<kernel::AccelNode>(),
        .bboxes = accel_bboxes_buffer_->TypedGpuData<kernel::Bbox>(),
        .instances = accel_instances_buffer_->TypedGpuData<kernel::AccelTop::Instance>(),
    };
    if (!accel_buffer_) {
        accel_buffer_ = std::make_unique<CuBuffer>(sizeof(accel), &accel);
    } else {
        accel_buffer_->SetData(&accel, sizeof(accel));
    }
}

void PathTracer::BuildInstancesAndLights() {
    std::vector<kernel::Instance> instances;
    std::vector<kernel::Light> lights;
    geo_light_buffers_.clear();

    scene_.ForEach<const MeshComponent, MaterialComponent>(
        [this, &instances, &lights](SceneObject &object, const MeshComponent &mesh, MaterialComponent &material) {
            kernel::Instance inst {
                .geometry = {
                    .type = kernel::Geometry::Type::eTriMesh,
                    .ptr = mesh.GetMesh()->GeometryBuffer()->GpuData(),
                },
                .material = std::bit_cast<kernel::Material>(material.GetMaterial()->Pointer()),
                .light = {
                    .type = kernel::Light::Type::eGeometry,
                    .ptr = nullptr,
                },
            };

            if (material.GetMaterial()->IsEmissive()) {
                auto trans = object.GetTransform();

                kernel::GeometryLight geo_light {
                    .geometry = inst.geometry,
                    .material = inst.material,
                    .transform = trans,
                    .transform_it = glm::transpose(glm::inverse(trans)),
                };
                auto geo_light_buffer = std::make_unique<CuBuffer>(sizeof(geo_light), &geo_light);
                inst.light.ptr = geo_light_buffer->GpuData();
                lights.push_back(inst.light);
                geo_light_buffers_.emplace_back(std::move(geo_light_buffer));
            }

            instances.push_back(inst);
        }
    );

    // scene_.ForEach<LightComponent>()

    num_lights_ = lights.size();

    auto instance_buffer_size = sizeof(kernel::Instance) * instances.size();
    if (!instances_buffer_ || instances_buffer_->Size() < instance_buffer_size) {
        instances_buffer_ = std::make_unique<CuBuffer>(instance_buffer_size, instances.data());
    } else {
        instances_buffer_->SetData(instances.data(), instance_buffer_size);
    }

    auto light_buffer_size = sizeof(kernel::Light) * lights.size();
    if (!lights_buffer_ || lights_buffer_->Size() < light_buffer_size) {
        lights_buffer_ = std::make_unique<CuBuffer>(light_buffer_size, lights.data());
    } else {
        lights_buffer_->SetData(lights.data(), light_buffer_size);
    }
}
