#pragma once

#include <string>

#include "shaper.h"
#include "lib_opts/skia.h"
#include "lib_opts/freetype.h"
#include "lib_opts/common.h"
#include "path.h"

#include <include/core/SkData.h>
#include <include/core/SkImage.h>
#include <include/core/SkStream.h>
#include <include/core/SkSurface.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkTextBlob.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkFontMgr.h>


using TextPaths = std::pair<std::vector<Path>, std::vector<float>>;


enum class RenderMode {
    FREETYPE,
    SKIA
};


class Renderer {
public:
    void set_font(const std::string& font_path);
    void set_text(const std::string& text) { return shaper.set_text(text); };
    TextPaths text_paths();
    ImageData render_text(unsigned font_size, RenderMode mode);
    std::vector<std::string> cluster_strings() { return shaper.cluster_strings(); };
private:
    Shaper shaper;
    lib_opts::Skia sk;

    std::vector<uint8_t> font_data;
};