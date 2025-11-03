#pragma once

#include "shaper.h"
#include "freetype.h"
#include "myfonts.h"
#include "path.h"

#include <string>


using TextPaths = std::pair<std::vector<Path>, std::vector<float>>;


enum class RenderMode {
    FREETYPE,
    MYFONTS
};


class Renderer {
public:
    void set_font(const std::string& font_path);
    void set_text(const std::string& text) { return shaper.set_text(text); };
    TextPaths text_paths();
    ImageData render_text(unsigned font_size, RenderMode mode, std::optional<std::string> myfonts_id);
    void shape(unsigned font_size, unsigned dpi) { shaper.shape(font_size, dpi); };

    int max_advance() const { return shaper.max_advance(); };
    std::vector<ClusterWindow> cluster_windows() const { return shaper.get_cluster_windows(); };
    std::vector<std::string> cluster_strings() const { return shaper.cluster_strings(); };
    void shape_if_needed() { if (shaper.get_clusters().empty()) shaper.shape_design(); };
private:
    Shaper shaper;
    std::vector<uint8_t> font_data;
};