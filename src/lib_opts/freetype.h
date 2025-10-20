#pragma once

#include "common.h"


namespace lib_opts {

class Freetype {
public:
    static ImageData render_text(const Shaper& shaper);
};

}