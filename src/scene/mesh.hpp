#pragma once

#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include "bbox.hpp"
#include "cuda_helpers/buffer.hpp"

class Mesh {
public:
    void SetPositions(std::vector<glm::vec3> &&positions);
    void SetNormals(std::vector<glm::vec3> &&normals);
    void SetTexcoords(std::vector<glm::vec2> &&texcoords);
    void SetIndices(std::vector<uint32_t> &&indices);

    void CalcNormals();
    void CalcTexcoords();

    size_t VericesCount() const { return positions_.size(); }
    const std::vector<glm::vec3> &Positions() const { return positions_; }
    const std::vector<glm::vec3> &Normals() const { return normals_; }
    const std::vector<glm::vec2> &Texcoords() const { return texcoords_; }
    size_t IndicesCount() const { return indices_.size(); }
    const std::vector<uint32_t> &Indices() const { return indices_; }

    const Bbox &Bbox() const { return bbox_; }

    CuBuffer *PositionBuffer() const { return positions_buffer_.get(); }
    CuBuffer *NormalBuffer() const { return normals_buffer_.get(); }
    CuBuffer *IndexBuffer() const { return indices_buffer_.get(); }

    CuBuffer *GeometryBuffer() const { return geometry_buffer_.get(); }

    void BuildAccel();
    CuBuffer *AccelBuffer() const { return accel_buffer_.get(); }

private:
    std::vector<glm::vec3> positions_;
    std::vector<glm::vec3> normals_;
    std::vector<glm::vec2> texcoords_;
    std::vector<uint32_t> indices_;
    struct Bbox bbox_;

    std::unique_ptr<CuBuffer> geometry_buffer_;
    std::unique_ptr<CuBuffer> positions_buffer_;
    std::unique_ptr<CuBuffer> normals_buffer_;
    std::unique_ptr<CuBuffer> texcoords_buffer_;
    std::unique_ptr<CuBuffer> indices_buffer_;

    std::unique_ptr<CuBuffer> accel_buffer_;
    std::unique_ptr<CuBuffer> accel_nodes_buffer_;
    std::unique_ptr<CuBuffer> accel_bboxes_buffer_;
};

class MeshComponent {
public:
    void SetMesh(std::shared_ptr<Mesh> mesh) { mesh_ = mesh; }
    Mesh *GetMesh() const { return mesh_.get(); }

private:
    std::shared_ptr<Mesh> mesh_;
};
