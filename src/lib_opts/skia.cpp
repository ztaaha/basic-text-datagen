#include "skia.h"

#include <stdexcept>

namespace lib_opts {

Skia::Skia() {
    mgr = SkFontMgr_New_DirectWrite(); // TODO: platform-specific
}

Skia::~Skia() {
    done_font();
}

void Skia::done_font() {
    typeface.reset();
    font_data.reset();
}


void Skia::set_font(const std::vector<uint8_t>& data) {
    font_data = SkData::MakeWithoutCopy(data.data(), data.size());
    if (!font_data)
        throw std::runtime_error("Couldn't find font");
    typeface = mgr->makeFromData(font_data);
    if (!typeface)
        throw std::runtime_error("Couldn't load skia font from data");
}

ImageData Skia::render_text(const Shaper& shaper, const unsigned font_size) const {
    const auto size = static_cast<float>(font_size);
    SkFont font(typeface, size);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    font.setSubpixel(true);
    font.setHinting(SkFontHinting::kNormal);
    font.setForceAutoHinting(false);

    SkTextBlobBuilder builder;
    const auto& run_buffer = builder.allocRunPos(font, static_cast<int>(shaper.get_glyph_count()));

    float x_adv = 0;
    for (const auto& [_, glyphs] : shaper.get_clusters()) {
        for (const unsigned glyph_id : glyphs) {
            run_buffer.glyphs[glyph_id] = shaper.get_glyph_info()[glyph_id].codepoint;
            const auto& pos = shaper.get_glyph_pos()[glyph_id];
            reinterpret_cast<SkPoint*>(run_buffer.pos)[glyph_id] =
                SkPoint::Make(
                    x_adv + static_cast<SkScalar>(pos.x_offset) / 64.0f,
                    static_cast<SkScalar>(-pos.y_offset) / 64.0f
                );
            x_adv += static_cast<SkScalar>(pos.x_advance) / 64.0f;
        }
    }
    const sk_sp<SkTextBlob> blob = builder.make();
    const SkRect& bounds = blob->bounds();
    const int left = SkScalarFloorToInt(bounds.left());
    const int top = SkScalarFloorToInt(bounds.top());
    const int right = SkScalarCeilToInt(bounds.right());
    const int bottom = SkScalarCeilToInt(bounds.bottom());
    const int width = right - left;
    const int height = bottom - top;
    
    const sk_sp<SkSurface> surface = SkSurfaces::Raster(
        SkImageInfo::Make(
            width,
            height,
            kN32_SkColorType,
            kOpaque_SkAlphaType,
            SkColorSpace::MakeSRGB()
        )
    );

    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    SkPaint p;
    p.setColor(SK_ColorBLACK);
    p.setAntiAlias(true);
    
    canvas->drawTextBlob(blob, SkIntToScalar(-left), SkIntToScalar(-top), p);


    SkPixmap pixmap;
    if (!surface->peekPixels(&pixmap))
        throw std::runtime_error("Couldn't peek pixels from Skia surface");
    const auto c = static_cast<Eigen::Index>(IMAGE_DIM + shaper.get_clusters().size());
    ImageData img(c, height, width);
    img.setZero();

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const SkColor pixel = pixmap.getColor(x, y);
            const uint8_t r = SkColorGetR(pixel);
            img(0, y, x) = r;
        }
    }

    x_adv = 0;
    for (unsigned i = 0; i < shaper.get_clusters().size(); i++) {
        const auto& [_, glyphs] = shaper.get_clusters()[i];
        SkTextBlobBuilder cluster_builder;
        const auto& run_buffer_cluster = cluster_builder.allocRunPos(font, static_cast<int>(glyphs.size()));

        int cluster_glyph_idx = 0;
        for (const unsigned glyph_id : glyphs) {
            run_buffer_cluster.glyphs[cluster_glyph_idx] = shaper.get_glyph_info()[glyph_id].codepoint;
            const auto& pos = shaper.get_glyph_pos()[glyph_id];
            reinterpret_cast<SkPoint*>(run_buffer_cluster.pos)[cluster_glyph_idx] =
                SkPoint::Make(
                    x_adv + static_cast<SkScalar>(pos.x_offset) / 64.0f,
                    static_cast<SkScalar>(-pos.y_offset) / 64.0f
                );
            x_adv += static_cast<SkScalar>(pos.x_advance) / 64.0f;
            cluster_glyph_idx++;
        }
        const sk_sp<SkTextBlob> blob_cluster = cluster_builder.make();
        canvas->clear(SK_ColorWHITE);
        canvas->drawTextBlob(blob_cluster, SkIntToScalar(-left), SkIntToScalar(-top), p);
        if (!surface->peekPixels(&pixmap))
            throw std::runtime_error("Couldn't peek pixels from Skia surface");
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                const SkColor pixel = pixmap.getColor(x, y);
                const uint8_t r = SkColorGetR(pixel);
                if (r < 255)
                    img(IMAGE_DIM + i, y, x) = 1;
            }
        }
    }

    return img;
}

}
