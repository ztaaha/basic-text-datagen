#include "render.h"

#include <fstream>
#include <optional>

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

ImageData Renderer::render_text(const unsigned font_size, const RenderMode mode, std::optional<std::string> myfonts_id) {
    ImageData img;
    switch (mode) {
        case RenderMode::FREETYPE:
            shaper.shape(font_size, 72);
            img = Freetype::render_text(shaper);
            break;
        case RenderMode::MYFONTS:
            shaper.shape(font_size, 96);
            img = MyFonts::render_text(shaper, font_size, *myfonts_id);
            break;
    }
    return img;
}
