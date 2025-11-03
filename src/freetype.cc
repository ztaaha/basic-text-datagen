#include "freetype.h"


#include <ft2build.h>
#include FT_GLYPH_H


inline int div255(const int v) {
    return ((v >> 8) + v) >> 8;
}



ImageData Freetype::render_text(const Shaper& shaper) {
    const auto [x_min, x_max, y_min, y_max] = shaper.text_size();
    const auto h = static_cast<Eigen::Index>(y_max - y_min);
    const auto w = static_cast<Eigen::Index>(x_max - x_min);
    const auto c = static_cast<Eigen::Index>(IMAGE_DIM + shaper.get_clusters().size());
    ImageData img(c, h, w);
    img.setZero();

    img.chip<0>(0).setConstant(255);

    int x = 0;
    for (unsigned i = 0; i < shaper.get_clusters().size(); i++) {
        const auto& [_, cluster] = shaper.get_clusters()[i];
        for (const unsigned glyph_id : cluster) {
            if (FT_Load_Glyph(shaper.get_ft_face(), shaper.get_glyph_info()[glyph_id].codepoint, FT_LOAD_RENDER))
                throw std::runtime_error("Glyph didn't load, render pass");

            const auto& pos = shaper.get_glyph_pos()[glyph_id];
            const auto& bitmap = shaper.get_ft_face()->glyph->bitmap;

            const int px = pixel(x + pos.x_offset);
            const int py = pixel(pos.y_offset);

            const unsigned pos_x = -x_min + px + shaper.get_ft_face()->glyph->bitmap_left;
            const unsigned pos_y = -(-y_max + py + shaper.get_ft_face()->glyph->bitmap_top);
            for (unsigned row = 0; row < bitmap.rows; row++) {
                for (unsigned col = 0; col < bitmap.width; col++) {
                    // if (glyph_alpha > 0) {
                    //     int current_alpha = img(gpy, gpx);
                    //     img(gpy, gpx) = current_alpha > 0 ? glyph_alpha + div255(current_alpha * (255 - glyph_alpha) + 128) : glyph_alpha;
                    // }
                    const int glyph_alpha = bitmap.buffer[row * bitmap.pitch + col];
                    if (glyph_alpha == 0)
                        continue;
                    const unsigned gpx = pos_x + col;
                    const unsigned gpy = pos_y + row;

                    img(IMAGE_DIM + i, gpy, gpx) = 1;

                    uint8_t& current_alpha = img(0, gpy, gpx);
                    current_alpha = div255(static_cast<int>(current_alpha) * (255 - glyph_alpha) + 128);
                }
            }
            x += pos.x_advance;
        }
    }

    return img;
}
