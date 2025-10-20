#pragma once

#include <map>
#include <string>
#include <vector>

#include <hb.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "path.h"


class Shaper {
public:
    Shaper();
    ~Shaper();

    const hb_glyph_info_t* get_glyph_info() const { return glyph_info; }
    const hb_glyph_position_t* get_glyph_pos() const { return glyph_pos; }
    unsigned get_glyph_count() const { return glyph_count; }
    const std::vector<std::pair<unsigned, std::vector<unsigned>>>& get_clusters() const { return clusters; }
    FT_Face get_ft_face() const { return face; }

    void set_font(const std::vector<uint8_t>& data);
    void set_text(const std::string& text);
    void shape_design();
    void shape(unsigned font_size);
    void path_data(std::vector<Path>& paths, std::vector<float>& advances);
    void done_font();
    std::vector<std::string> cluster_strings() const;
private:
    void shape_internal();


    FT_Library library = nullptr;
    FT_Face face = nullptr;

    hb_buffer_t* buf;
    hb_font_t* font = nullptr;
    unsigned glyph_count = 0;
    hb_glyph_info_t* glyph_info = nullptr;
    hb_glyph_position_t* glyph_pos = nullptr;

    std::string text;
    std::vector<std::pair<unsigned, std::vector<unsigned>>> clusters;
};