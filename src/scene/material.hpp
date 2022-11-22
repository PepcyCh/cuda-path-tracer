#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "cuda_helpers/buffer.hpp"
#include "cuda_helpers/texture.hpp"
#include "kernels/basic/prelude.cuh"

class Material {
public:
    CuBuffer *Buffer() const { return buffer_.get(); }
    const kernel::TaggedPointer &Pointer() const { return ptr_; }

    bool IsEmissive() const;

    void SetChanged();

    void ShowUi();

    glm::vec3 emission = glm::vec3(0.0f);
    glm::vec3 diffuse = glm::vec3(0.5f);
    glm::vec3 specular = glm::vec3(0.5f);
    glm::vec3 transmittance = glm::vec3(0.0f);
    float ior = 1.5f;
    float shininess = 1.0f;
    float opacity = 1.0f;
    int bsdf_type = 0;

    std::unique_ptr<CuTexture> emission_map;
    std::unique_ptr<CuTexture> diffuse_map;
    std::unique_ptr<CuTexture> specular_map;

private:
    void BuildBuffer();

    std::unique_ptr<CuBuffer> buffer_;
    kernel::TaggedPointer ptr_;
};

class MaterialComponent {
public:
    void SetMaterial(std::shared_ptr<Material> material) { material_ = material; }
    Material *GetMaterial() const { return material_.get(); }

    void ShowUi();

private:
    std::shared_ptr<Material> material_;
};
