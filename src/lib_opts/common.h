#pragma once

#include <Eigen/Dense>
#include <unsupported/Eigen/CXX11/Tensor>

#include "../shaper.h"

using ImageData = Eigen::Tensor<uint8_t, 3, Eigen::RowMajor>;
static constexpr int IMAGE_DIM = 1;

struct TextBox {
    int x_min;
    int x_max;
    int y_min;
    int y_max;

    static TextBox extremes() {
        return {std::numeric_limits<int>::max(), std::numeric_limits<int>::min(),
                std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
    }
};