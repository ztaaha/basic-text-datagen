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

void Renderer::set_mode(RenderMode mode, std::optional<std::string> myfonts_id) {
    this->mode = mode;
    this->myfonts_id = std::move(myfonts_id);

    Params p;
    p.disable_features = (mode == RenderMode::MYFONTS);
    p.dpi = (mode == RenderMode::MYFONTS) ? 96 : 72;

    shaper.set_params(p);
}

TextPaths Renderer::text_paths() {
    shaper.shape_design();
    std::vector<Path> paths;
    std::vector<float> advances;
    shaper.path_data(paths, advances);
    return {paths, advances};
}

ImageData Renderer::render_text(const unsigned font_size) {
    ImageData img;
    shaper.shape(font_size);
    switch (mode) {
        case RenderMode::FREETYPE:
            img = Freetype::render_text(shaper);
            break;
        case RenderMode::MYFONTS:
            img = MyFonts::render_text(shaper, font_size, *myfonts_id);
            break;
        default:
            throw std::runtime_error("Shielded by Python");
    }
    return img;
}
