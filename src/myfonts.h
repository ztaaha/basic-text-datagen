#pragma once

#include "shaper.h"
#include "common.h"
#include "owned_image.h"

#include <cpr/cpr.h>
#include <optional>

struct ClusterPair {
    std::string text;
    bool last;
};

struct Responses {
    cpr::AsyncResponse first;
    std::optional<cpr::AsyncResponse> second;
};


class MyFonts {
public:
    static ImageData render_text(const Shaper& shaper, unsigned font_size, const std::string& myfonts_id);
};