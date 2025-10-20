#pragma once

#include <include/core/SkData.h>
#include <include/core/SkImage.h>
#include <include/core/SkStream.h>
#include <include/core/SkSurface.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkTextBlob.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkColorSpace.h>
#include <include/ports/SkTypeface_win.h>
#include <include/core/SkColorFilter.h>
#include <include/effects/SkColorMatrix.h>

#include "common.h"

namespace lib_opts {

class Skia {
public:
    Skia();
    ~Skia();

    void set_font(const std::vector<uint8_t>& data);
    void done_font();
    ImageData render_text(const Shaper& shaper, unsigned font_size) const;
private:
    sk_sp<SkFontMgr> mgr;
    sk_sp<SkTypeface> typeface;
    sk_sp<SkData> font_data;
};

}
