#include "render.h"

#include <fstream>

void Renderer::set_font(const std::string& font_path) {
    shaper.done_font();

    std::ifstream font(font_path, std::ios::binary);
    if (!font.is_open())
        throw std::runtime_error("Invalid font: " + font_path);
    font_data.assign(
        (std::istreambuf_iterator<char>(font)),
        (std::istreambuf_iterator<char>())
    );
    font.close();

    shaper.set_font(font_data);
}

TextPaths Renderer::text_paths() {
    shaper.shape_design();
    std::vector<Path> paths;
    std::vector<float> advances;
    shaper.path_data(paths, advances);
    return {paths, advances};
}

ImageData Renderer::render_text(const unsigned font_size) {
    shaper.shape(font_size);
    ImageData img = Freetype::render_text(shaper);
    return img;
}
