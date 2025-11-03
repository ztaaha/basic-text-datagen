#pragma once

#include <Eigen/Dense>
#include <unsupported/Eigen/CXX11/Tensor>


using ImageData = Eigen::Tensor<uint8_t, 3, Eigen::RowMajor>;
using ImageTensor = Eigen::Tensor<uint8_t, 2, Eigen::RowMajor>;
static constexpr int IMAGE_DIM = 1;

inline int pixel(const int c) {
    return ((c + 32) & ~63) >> 6;
}


struct TextBox {
    int x_min;
    int x_max;
    int y_min;
    int y_max;
};
