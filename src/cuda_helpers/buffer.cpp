#include "buffer.hpp"

#include <cstdint>

#include <cuda_runtime.h>

CuBuffer::CuBuffer(size_t size, const void *init_data) : size_(size) {
    cudaMalloc(&buffer_, size);
    if (init_data) {
        SetData(init_data, size);
    }
}

CuBuffer::~CuBuffer() {
    cudaFree(buffer_);
}

void CuBuffer::SetData(const void *data, size_t size, size_t offset) {
    cudaMemcpy(reinterpret_cast<uint8_t *>(buffer_) + offset, data, size, cudaMemcpyHostToDevice);
}
