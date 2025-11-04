#pragma once

#include "shaper.h"
#include "freetype.h"
#include "myfonts.h"
#include "path.h"

#include <string>


using TextPaths = std::pair<std::vector<Path>, std::vector<float>>;


enum class RenderMode {
    FREETYPE,
    MYFONTS,
    OTHER
};


class Renderer {
public:
    void set_font(const std::string& font_path);
    void set_text(const std::string& text) { return shaper.set_text(text); };
    void set_mode(RenderMode mode, std::optional<std::string> myfonts_id);
    TextPaths text_paths();
    ImageData render_text(unsigned font_size);


    // Needed for web rendering in Python
    std::vector<std::string> cluster_strings() const { return shaper.cluster_strings(); };
    void shape_if_needed() { if (shaper.get_clusters().empty()) shaper.shape_design(); };
private:

    Shaper shaper;
    std::vector<uint8_t> font_data;

    RenderMode mode;
    std::optional<std::string> myfonts_id;
};