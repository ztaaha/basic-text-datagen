#pragma once

#include "common.h"

#include <stb_image.h>

class OwnedImage {
private:
    unsigned char* data;
    int height;
    int width;

public:
    OwnedImage(unsigned char* data, int height, int width)
        : data(data), height(height), width(width) {
    }

    ~OwnedImage() {
        if (data)
            stbi_image_free(data);
    }

    OwnedImage(const OwnedImage&) = delete;
    OwnedImage& operator=(const OwnedImage&) = delete;

    OwnedImage(OwnedImage&& other) noexcept
        : data(other.data), height(other.height), width(other.width) {
        other.data = nullptr;
    }

    Eigen::TensorMap<ImageTensor> view() {
        return Eigen::TensorMap<ImageTensor>(data, height, width);
    }

    int h() const { return height; }
    int w() const { return width; }
    std::array<int, 2> dims() const { return {height, width}; }
};