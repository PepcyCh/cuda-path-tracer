#pragma once

#include <glm/glm.hpp>
#include <texture_types.h>

class Film {
public:
    Film(uint32_t width, uint32_t height);
    ~Film();

    uint32_t Width() const { return width_; }
    uint32_t Height() const { return height_; }
    void Resize(uint32_t width, uint32_t height);

    glm::vec4 *CudaMap();
    void CudaUnmap();

    uint32_t GlTexture() const { return gl_texture_; }

private:
    void Init(uint32_t width, uint32_t height);

    uint32_t gl_buffer_ = 0;
    uint32_t gl_texture_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    cudaGraphicsResource_t cuda_res_ = nullptr;
};
