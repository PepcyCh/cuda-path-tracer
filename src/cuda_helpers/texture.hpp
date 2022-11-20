#pragma once

#include <cstdint>

#include <texture_types.h>

class CuTexture {
public:
    CuTexture(bool is_srgb, uint32_t width, uint32_t height, uint32_t bytes_per_row, const void *data);
    ~CuTexture();

    cudaTextureObject_t Object() const { return texture_; }

private:
    cudaArray_t array_;
    cudaTextureObject_t texture_;
};
