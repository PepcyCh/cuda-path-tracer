#pragma once

class CuBuffer {
public:
    CuBuffer(size_t size, const void *init_data = nullptr);
    ~CuBuffer();

    CuBuffer(const CuBuffer &rhs) = delete;
    CuBuffer &operator=(const CuBuffer &rhs) = delete;

    void SetData(const void *data, size_t size, size_t offset = 0);

    void *GpuData() const { return buffer_; }
    template <typename T>
    T *TypedGpuData() const { return reinterpret_cast<T *>(GpuData()); }

    size_t Size() const { return size_; }

private:
    void *buffer_ = nullptr;
    size_t size_ = 0;
};
