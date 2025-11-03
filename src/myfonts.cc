#define STB_IMAGE_IMPLEMENTATION
#include "myfonts.h"
#include <fmt/format.h>
#include <algorithm>



inline cpr::Url get_url(const std::string& myfonts_id) {
    return cpr::Url{fmt::format(
        "https://sig.monotype.com/render/105/font/{}",
        myfonts_id
    )};
}

inline cpr::Parameters get_params(const std::string& text, const unsigned font_size, const unsigned spacing) {
    return cpr::Parameters{
        {"rt", text},
        {"rs", std::to_string(font_size)},
        {"w", "4000"},
        {"fg", "000000"},
        {"bg", "FFFFFF"},
        {"t", "o"},
        {"sc", "1"},
        {"userLang", "en"},
        {"render_mode", "new"},
        {"tr", std::to_string(spacing)}
    };
}


OwnedImage load_img(const cpr::Response& res) {
    if (res.status_code != 200)
        throw std::runtime_error(fmt::format("MyFonts request failed with status: {}", res.status_code));
    int width, height, comp;
    unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(res.text.data()), res.text.size(), &width, &height, &comp, 1);
    if (!data)
        throw std::runtime_error("Failed to load image from MyFonts response");
    return {data, height, width};
}


std::vector<Responses> queue_requests(const std::vector<ClusterPair>& cluster_pairs, const unsigned font_size, const std::string& myfonts_id, const Shaper& shaper, const unsigned max_cluster_width) {
    std::vector<Responses> responses;
    responses.reserve(IMAGE_DIM + cluster_pairs.size());
    const cpr::Url base = get_url(myfonts_id);
    responses.emplace_back(cpr::GetAsync(base, get_params(shaper.get_text(), font_size, 0)), std::nullopt);
    for (const auto& [text, last] : cluster_pairs) {
        Responses r;
        r.first = cpr::GetAsync(base, get_params(text, font_size, 0));
        if (!last)
            r.second = cpr::GetAsync(base, get_params(text, font_size, max_cluster_width));
        responses.emplace_back(std::move(r));
    }
    return responses;
}

template<typename Tensor>
void invert_inplace(Tensor&& img) {
    const auto size = img.size();
    auto* ptr = img.data();
    for (size_t i = 0; i < size; ++i) {
        ptr[i] = 255 - ptr[i];
    }
}


template<typename Image, typename Dims>
std::pair<Eigen::array<Eigen::Index, 2>, Eigen::array<Eigen::Index, 2>> nonzero(const Image& img, const Dims& dims, const bool wonb = true) {
    TextBox box{};

    const uint8_t bg = wonb ? 0 : 255;

    for (int y = 0; y < dims[0]; ++y) {
        for (int x = 0; x < dims[1]; ++x) {
            if (img(y, x) != bg) {
                box.y_min = y;
                goto found_top;
            }
        }
    }
found_top:
    for (int y = dims[0] - 1; y >= 0; --y) {
        for (int x = 0; x < dims[1]; ++x) {
            if (img(y, x) != bg) {
                box.y_max = y;
                goto found_bottom;
            }
        }
    }
found_bottom:
    for (int x = 0; x < dims[1]; ++x) {
        for (int y = 0; y < dims[0]; ++y) {
            if (img(y, x) != bg) {
                box.x_min = x;
                goto found_left;
            }
        }
    }
found_left:
    for (int x = dims[1] - 1; x >= 0; --x) {
        for (int y = 0; y < dims[0]; ++y) {
            if (img(y, x) != bg) {
                box.x_max = x;
                goto found_right;
            }
        }
    }
found_right:

    Eigen::array<Eigen::Index, 2> offsets = {box.y_min, box.x_min};
    Eigen::array<Eigen::Index, 2> extents = {box.y_max - box.y_min + 1, box.x_max - box.x_min + 1};
    return {offsets, extents};
}







void fill_cluster_mask(
            ImageData& img_data,
            const unsigned channel,
            const ImageTensor& cluster_img,
            const ImageTensor& full_img,
            const int window_start,
            const int window_end
        ) {
    const int th = cluster_img.dimension(0);
    const int tw = cluster_img.dimension(1);
    const int fh = full_img.dimension(0);
    const int fw = full_img.dimension(1);
    const int start_x = std::max(0, window_start - tw + 1);
    const int end_x = std::min(fw - tw, window_end - 1);
    constexpr int start_y = 0;
    const int end_y = fh - th;

    float min_loss = std::numeric_limits<float>::infinity();
    int best_y = 0;
    int best_x = 0;

    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            const int tx_start = std::max(0, window_start - x);
            const int tx_end = std::min(tw, window_end - x);

            float sqdiff = 0.0f;
            for (int ty = 0; ty < th; ++ty) {
                for (int tx = tx_start; tx < tx_end; ++tx) {
                    // assert(tx >= 0 && tx < tw);
                    // assert(ty >= 0 && ty < th);
                    if (!(tx >= 0 && tx < tw))
                        throw std::runtime_error("tx out of bounds");
                    if (!(ty >= 0 && ty < th))
                        throw std::runtime_error("ty out of bounds");
                    if (cluster_img(ty, tx) != 0) {
                        const float diff = cluster_img(ty, tx) - full_img(y + ty, x + tx);
                        sqdiff += diff * diff;
                    }
                }
            }

            if (sqdiff < min_loss) {
                min_loss = sqdiff;
                best_y = y;
                best_x = x;
            }
        }
    }

    for (int y = 0; y < th; ++y) {
        for (int x = 0; x < tw; ++x) {
            img_data(channel, best_y + y, best_x + x) = cluster_img(y, x) > 0 ? 1 : 0;
        }
    }

}




ImageData MyFonts::render_text(const Shaper& shaper, const unsigned font_size, const std::string& myfonts_id) {
    std::vector<ClusterPair> urls_to_get;
    const std::vector<std::string> strings = shaper.cluster_strings();
    const std::vector<ClusterWindow> windows = shaper.get_cluster_windows();
    unsigned max_width;
    shaper.text_size(&max_width);




    urls_to_get.reserve(strings.size());
    for (size_t i = 0; i < strings.size(); ++i) {
        ClusterPair p;
        const std::string& first = strings[i];
        if (i < strings.size() - 1) {
            p.text = first + strings[i + 1];
            p.last = false;
        } else {
            p.text = first;
            p.last = true;
        }
        urls_to_get.emplace_back(std::move(p));
    }
    std::vector<Responses> responses = queue_requests(urls_to_get, font_size, myfonts_id, shaper, static_cast<unsigned>(max_width * 1.5f)); // extra gap just in case



    OwnedImage img_og{load_img(responses[0].first.get())};
    const auto nz_full = nonzero(img_og.view(), img_og.dims(), false);

    const auto c = static_cast<Eigen::Index>(IMAGE_DIM + strings.size());
    ImageData img_data(c, nz_full.second[0], nz_full.second[1]);
    img_data.setZero();

    ImageTensor img = img_og.view().slice(nz_full.first, nz_full.second);
    img_data.chip<0>(0) = img;
    invert_inplace(img);




    for (int i = IMAGE_DIM; i < responses.size(); ++i) {
        auto& [unspaced_res, spaced_res] = responses[i];
        OwnedImage unspaced{load_img(unspaced_res.get())};
        invert_inplace(unspaced.view());

        const ClusterWindow& window = windows[i - IMAGE_DIM];

        if (spaced_res.has_value()) {
            OwnedImage spaced{load_img(spaced_res->get())};
            invert_inplace(spaced.view());

            int x = 0;
            for (; x < spaced.w(); ++x) {
                for (int y = 0; y < spaced.h(); ++y) {
                    if (spaced.view()(y, x) > unspaced.view()(y, x))
                        goto found_x;
                }
            }
        found_x:

            Eigen::array<Eigen::Index, 2> search_dims({spaced.h(), x});
            const auto& [offsets, extents] = nonzero(spaced.view(), search_dims);


            ImageTensor cluster = spaced.view().slice(offsets, extents);
            fill_cluster_mask(img_data, i, cluster, img, window.x, window.end);


        } else {
            const auto& [offsets, extents] = nonzero(
                unspaced.view(), unspaced.dims()
            );
            ImageTensor cluster = unspaced.view().slice(offsets, extents);

            fill_cluster_mask(img_data, i, cluster, img, window.x, img.dimension(1));
        }
    }


    return img_data;
}
