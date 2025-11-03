#pragma once

#include "shaper.h"
#include "common.h"


#include <limits>



class Freetype {
public:
    static ImageData render_text(const Shaper& shaper);
};
