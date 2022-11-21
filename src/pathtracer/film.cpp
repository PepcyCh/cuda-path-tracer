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
    glBindTexture(GL_TEXTURE_2D, gl_texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void Film::Init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    glGenBuffers(1, &gl_buffer_);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_buffer_);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glGenTextures(1, &gl_texture_);
    glBindTexture(GL_TEXTURE_2D, gl_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    cudaGraphicsGLRegisterBuffer(&cuda_res_, gl_buffer_, cudaGraphicsRegisterFlagsNone);
}
