#include "mesh.hpp"

#include <numbers>

#include "kernels/accel/accel_build.cuh"

void Mesh::SetPositions(std::vector<glm::vec3> &&positions) {
    positions_ = std::move(positions);
    bbox_.Empty();
    for (const auto &pos : positions_) {
        bbox_.Merge(pos);
    }

    auto buffer_size = sizeof(glm::vec3) * positions_.size();
    if (!positions_buffer_ || positions_buffer_->Size() < buffer_size) {
        positions_buffer_ = std::make_unique<CuBuffer>(buffer_size, positions_.data());
    } else {
        positions_buffer_->SetData(positions_.data(), buffer_size);
    }
}

void Mesh::SetNormals(std::vector<glm::vec3> &&normals) {
    normals_ = std::move(normals);

    auto buffer_size = sizeof(glm::vec3) * normals_.size();
    if (!normals_buffer_ || normals_buffer_->Size() < buffer_size) {
        normals_buffer_ = std::make_unique<CuBuffer>(buffer_size, normals_.data());
    } else {
        normals_buffer_->SetData(normals_.data(), buffer_size);
    }
}

void Mesh::SetTexcoords(std::vector<glm::vec2> &&texcoords) {
    texcoords_ = std::move(texcoords);

    auto buffer_size = sizeof(glm::vec2) * texcoords_.size();
    if (!texcoords_buffer_ || texcoords_buffer_->Size() < buffer_size) {
        texcoords_buffer_ = std::make_unique<CuBuffer>(buffer_size, texcoords_.data());
    } else {
        texcoords_buffer_->SetData(texcoords_.data(), buffer_size);
    }
}

void Mesh::SetIndices(std::vector<uint32_t> &&indices) {
    indices_ = std::move(indices);

    auto buffer_size = sizeof(uint32_t) * indices_.size();
    if (!indices_buffer_ || indices_buffer_->Size() < buffer_size) {
        indices_buffer_ = std::make_unique<CuBuffer>(buffer_size, indices_.data());
    } else {
        indices_buffer_->SetData(indices_.data(), buffer_size);
    }
}

void Mesh::CalcNormals() {
    normals_.resize(positions_.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices_.size(); i += 3) {
        auto p0 = positions_[indices_[i]];
        auto p1 = positions_[indices_[i + 1]];
        auto p2 = positions_[indices_[i + 2]];
        auto n = glm::normalize(glm::cross(p1 - p0, p2 - p0));
        normals_[indices_[i]] += n;
        normals_[indices_[i + 1]] += n;
        normals_[indices_[i + 2]] += n;
    }
    for (auto &norm : normals_) {
        norm = glm::normalize(norm);
    }
}

void Mesh::CalcTexcoords() {
    texcoords_.resize(positions_.size());
    for (size_t i = 0; i < positions_.size(); i++) {
        auto vec = glm::normalize(positions_[i] - bbox_.Centroid());
        auto theta = std::acos(vec.y);
        auto phi = std::atan2(vec.z, vec.x);
        if (phi < 0.0f) {
            phi += 2.0f * std::numbers::pi;
        }
        texcoords_[i] = glm::vec2(phi * std::numbers::inv_pi * 0.5f, 1.0f - theta * std::numbers::inv_pi);
    }
}

void Mesh::BuildAccel() {
    uint32_t num_triangles = indices_.size() / 3;
    uint32_t num_accel_nodes = num_triangles * 2 - 1;

    std::vector<kernel::Bbox> accel_bboxes(num_accel_nodes);
    for (size_t i = 0; i < num_triangles; i++) {
        auto i0 = indices_[3 * i];
        auto i1 = indices_[3 * i + 1];
        auto i2 = indices_[3 * i + 2];
        auto p0 = positions_[i0];
        auto p1 = positions_[i1];
        auto p2 = positions_[i2];
        accel_bboxes[num_triangles - 1 + i].pmin = glm::vec4(glm::min(p0, glm::min(p1, p2)), 1.0f);
        accel_bboxes[num_triangles - 1 + i].pmax = glm::vec4(glm::max(p0, glm::max(p1, p2)), 1.0f);
    }
    auto bbox_buffer_size = sizeof(kernel::Bbox) * num_accel_nodes;
    if (!accel_bboxes_buffer_ || accel_bboxes_buffer_->Size() < bbox_buffer_size) {
        accel_bboxes_buffer_ = std::make_unique<CuBuffer>(bbox_buffer_size, accel_bboxes.data());
    } else {
        accel_bboxes_buffer_->SetData(accel_bboxes.data(), bbox_buffer_size);
    }

    auto node_buffer_size = sizeof(kernel::AccelNode) * num_accel_nodes;
    if (!accel_nodes_buffer_ || accel_nodes_buffer_->Size() < node_buffer_size) {
        accel_nodes_buffer_ = std::make_unique<CuBuffer>(node_buffer_size);
    }

    kernel::Bbox merged_bbox {
        .pmin = glm::vec4(bbox_.pmin, 1.0f),
        .pmax = glm::vec4(bbox_.pmax, 1.0f),
    };

    kernel::BuildAccel(
        accel_nodes_buffer_->TypedGpuData<kernel::AccelNode>(),
        accel_bboxes_buffer_->TypedGpuData<kernel::Bbox>(),
        merged_bbox, num_triangles
    );


    kernel::TriMesh trimesh {
        .positions = positions_buffer_->TypedGpuData<glm::vec3>(),
        .normals = normals_buffer_->TypedGpuData<glm::vec3>(),
        .texcoords = texcoords_buffer_->TypedGpuData<glm::vec2>(),
        .indices = indices_buffer_->TypedGpuData<uint32_t>(),
        .num_triangles = num_triangles,
    };
    if (!geometry_buffer_) {
        geometry_buffer_ = std::make_unique<CuBuffer>(sizeof(trimesh), &trimesh);
    } else {
        geometry_buffer_->SetData(&trimesh, sizeof(trimesh));
    }

    kernel::AccelBottom accel {
        .nodes = accel_nodes_buffer_->TypedGpuData<kernel::AccelNode>(),
        .bboxes = accel_bboxes_buffer_->TypedGpuData<kernel::Bbox>(),
        .geometry = {
            .type = kernel::Geometry::Type::eTriMesh,
            .ptr = geometry_buffer_->GpuData(),
        }
    };
    if (!accel_buffer_) {
        accel_buffer_ = std::make_unique<CuBuffer>(sizeof(accel), &accel);
    } else {
        accel_buffer_->SetData(&accel, sizeof(accel));
    }
}
