// native/include/kindle/frame_differ.hpp
#pragma once
#include "kindle/eink.hpp"
#include "kindle/pixmap.hpp"
#include <cstdint>

namespace kindle {

class FrameDiffer {
public:
    // Compare two same-size 8bpp Pixmaps.
    // Returns nibble-aligned bounding box of changed pixels
    // expanded by ghosting_margin. Returns {0,0,0,0} if identical.
    static EinkRect compute(const Pixmap& prev, const Pixmap& current,
                            uint32_t ghosting_margin = 0);
};

} // namespace kindle
