#include "film.hpp"

#include <glad/glad.h>
#include <cuda_gl_interop.h>

Film::Film(uint32_t width, uint32_t height) {
    Init(width, height);
}

Film::~Film() {
    cudaGraphicsUnmapResources(1, &cuda_res_);
    glDeleteBuffers(1, &gl_buffer_);
    glDeleteTextures(1, &gl_texture_);
}

void Film::Resize(uint32_t width, uint32_t height) {
    if (width_ != width || height_ != height) {
        this->~Film();
        Init(width, height);
    }
}

glm::vec4 *Film::CudaMap() {
    cudaGraphicsMapResources(1, &cuda_res_);
    size_t size;
    void *ptr;
    cudaGraphicsResourceGetMappedPointer(&ptr, &size, cuda_res_);
    return reinterpret_cast<glm::vec4 *>(ptr);
}

void Film::CudaUnmap() {
    cudaGraphicsUnmapResources(1, &cuda_res_);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_buffer_);
    glTextureSubImage2D(gl_texture_, 0, 0, 0, width_, height_, GL_RGBA, GL_FLOAT, nullptr);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void Film::Init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    glCreateBuffers(1, &gl_buffer_);
    glNamedBufferStorage(gl_buffer_, width * height * sizeof(glm::vec4), nullptr, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &gl_texture_);
    glTextureStorage2D(gl_texture_, 1, GL_RGBA32F, width, height);

    cudaGraphicsGLRegisterBuffer(&cuda_res_, gl_buffer_, cudaGraphicsRegisterFlagsNone);
}
