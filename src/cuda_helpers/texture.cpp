#include "texture.hpp"

#include <channel_descriptor.h>

CuTexture::CuTexture(bool is_srgb, uint32_t width, uint32_t height, uint32_t bytes_per_row, const void *data) {
    cudaChannelFormatDesc channel_desc = cudaCreateChannelDesc<uchar4>();
    cudaMallocArray(&array_, &channel_desc, width, height);
    cudaMemcpy2DToArray(array_, 0, 0, data, bytes_per_row, bytes_per_row, height, cudaMemcpyHostToDevice);

    cudaResourceDesc res_desc {
        .resType = cudaResourceTypeArray,
        .res = {
            .array = { array_ }
        }
    };
    cudaTextureDesc tex_desc {
        .addressMode = { cudaAddressModeWrap, cudaAddressModeWrap, cudaAddressModeWrap },
        .filterMode = cudaFilterModeLinear,
        .readMode = cudaReadModeNormalizedFloat,
        .sRGB = is_srgb,
        .borderColor = { 1.0f, 1.0f, 1.0f, 1.0f },
        .normalizedCoords = 1,
        .maxAnisotropy = 1,
        .mipmapFilterMode = cudaFilterModeLinear,
    };
    cudaCreateTextureObject(&texture_, &res_desc, &tex_desc, nullptr);
}

CuTexture::~CuTexture() {
    cudaDestroyTextureObject(texture_);
    cudaFreeArray(array_);
}
