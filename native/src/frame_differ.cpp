// native/src/frame_differ.cpp
#include "kindle/frame_differ.hpp"
#include <algorithm>

namespace kindle {

EinkRect FrameDiffer::compute(const Pixmap& prev, const Pixmap& cur,
                               uint32_t margin) {
    uint32_t w = prev.width(), h = prev.height();
    uint32_t x0 = w, y0 = h, x1 = 0, y1 = 0;

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            if (prev.get_pixel(x, y) != cur.get_pixel(x, y)) {
                x0 = std::min(x0, x);
                y0 = std::min(y0, y);
                x1 = std::max(x1, x + 1);
                y1 = std::max(y1, y + 1);
            }
        }
    }

    if (x0 >= x1) return {0, 0, 0, 0};

    // Expand by ghosting margin
    uint32_t rx0 = (x0 >= margin ? x0 - margin : 0u) & ~1u;  // nibble align
    uint32_t ry0 = (y0 >= margin ? y0 - margin : 0u);
    uint32_t rx1 = std::min(x1 + margin, w);
    uint32_t ry1 = std::min(y1 + margin, h);

    return {rx0, ry0, rx1 - rx0, ry1 - ry0};
}

} // namespace kindle
