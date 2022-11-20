#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "cuda_helpers/buffer.hpp"
#include "cuda_helpers/texture.hpp"

class Material {
public:
    CuBuffer *Buffer() const { return buffer_.get(); }

    void BuildBuffer();

    bool IsEmissive() const;

    glm::vec3 emission = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(0.5f);
    std::unique_ptr<CuTexture> emission_map;
    std::unique_ptr<CuTexture> color_map;

private:
    std::unique_ptr<CuBuffer> buffer_;
};

class MaterialComponent {
public:
    void SetMaterial(std::shared_ptr<Material> material) { material_ = material; }
    Material *GetMaterial() const { return material_.get(); }

    void ShowUi();

private:
    std::shared_ptr<Material> material_;
};
