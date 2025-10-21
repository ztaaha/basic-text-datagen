#include "shaper.h"

#include <stdexcept>
#include <hb-ft.h>

Shaper::Shaper() {
    if (FT_Init_FreeType(&library)) throw std::runtime_error("Freetype library not init");
    buf = hb_buffer_create();
}

Shaper::~Shaper() {
    done_font();
    hb_buffer_destroy(buf);
    FT_Done_FreeType(library);
}

void Shaper::done_font() {
    if (font) hb_font_destroy(font);
    if (face) FT_Done_Face(face);
    font = nullptr;
    face = nullptr;
}

void Shaper::set_font(const std::vector<uint8_t>& data) {
    if (FT_New_Memory_Face(library, data.data(), static_cast<FT_Long>(data.size()), 0, &face)) throw std::runtime_error("Couldn't load FreeType font from data");
    font = hb_ft_font_create_referenced(face);
    clusters.clear();
}

void Shaper::set_text(const std::string& text) {
    this->text = text;
    clusters.clear();
}

void Shaper::shape_internal() {
    hb_buffer_reset(buf);
    hb_buffer_add_utf8(buf, text.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(buf);
    hb_shape(font, buf, nullptr, 0);
    glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);
    if (clusters.empty()) {
        std::map<unsigned, std::vector<unsigned>> cluster_map;
        for (int i = 0; i < glyph_count; i++) {
            const unsigned cluster = glyph_info[i].cluster;
            cluster_map[cluster].emplace_back(i);
        }
        clusters.assign(cluster_map.begin(), cluster_map.end());
    }
}


void Shaper::shape_design() {
    FT_Set_Char_Size(face, 0, face->units_per_EM * 64, 0, 0);
    hb_ft_font_changed(font);
    shape_internal();
}

void Shaper::shape(const unsigned font_size) {
    FT_Set_Pixel_Sizes(face, 0, font_size);
    hb_ft_font_changed(font);
    shape_internal();
}


void Shaper::path_data(std::vector<Path>& paths, std::vector<float>& advances) {
    for (unsigned i = 0; i < clusters.size(); i++) {
        const auto& [_, cluster] = clusters[i];
        Path path;
        float x = 0;
        for (const unsigned glyph_id : cluster ) {
            if (FT_Load_Glyph(face, glyph_info[glyph_id].codepoint, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP))
                throw std::runtime_error("Glyph didn't load, design pass");
            FT_Outline& outline = face->glyph->outline;

            const auto& pos = glyph_pos[glyph_id];
            const auto x_offset = static_cast<float>(pos.x_offset);
            const auto offset_x = x + x_offset;
            const auto offset_y = static_cast<float>(pos.y_offset);
            path.add(outline, {offset_x, offset_y});
            const auto x_advance = static_cast<float>(pos.x_advance);
            x += x_advance;
        }
        paths.push_back(std::move(path));
        if (i < clusters.size() - 1)
            advances.emplace_back(x / 64.0f);
    }
}


std::vector<std::string> Shaper::cluster_strings() const {
    std::vector<std::string> cluster_strs;
    for (unsigned i = 0; i < clusters.size(); i++) {
        const unsigned start = clusters[i].first;
        const unsigned end = i < clusters.size() - 1 ? clusters[i + 1].first : text.size();
        const unsigned len = end - start;
        cluster_strs.push_back(text.substr(start, len));
    }
    return cluster_strs;
}
