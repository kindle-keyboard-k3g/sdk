// native/src/dirty.cpp
#include "kindle/dirty.hpp"
#include <algorithm>

namespace kindle {

void DirtyRegion::mark(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    uint32_t ax = align_x(x);
    uint32_t nx0 = ax;
    uint32_t ny0 = y;
    uint32_t nx1 = ax + w + (x - ax);  // preserve right edge
    uint32_t ny1 = y + h;
    if (empty_) {
        x0_ = nx0; y0_ = ny0; x1_ = nx1; y1_ = ny1;
        empty_ = false;
    } else {
        x0_ = std::min(x0_, nx0);
        y0_ = std::min(y0_, ny0);
        x1_ = std::max(x1_, nx1);
        y1_ = std::max(y1_, ny1);
    }
}

void DirtyRegion::mark(const EinkRect& r) {
    mark(r.x, r.y, r.width, r.height);
}

void DirtyRegion::mark_all(uint32_t sw, uint32_t sh) {
    mark(0, 0, sw, sh);
}

void DirtyRegion::expand(uint32_t m, uint32_t sw, uint32_t sh) {
    if (empty_) return;
    x0_ = align_x((x0_ >= m) ? x0_ - m : 0u);
    y0_ = (y0_ >= m) ? y0_ - m : 0u;
    x1_ = std::min(x1_ + m, sw);
    y1_ = std::min(y1_ + m, sh);
}

EinkRect DirtyRegion::bounds() const {
    if (empty_) return {0, 0, 0, 0};
    return {x0_, y0_, x1_ - x0_, y1_ - y0_};
}

float DirtyRegion::area_ratio(uint32_t sw, uint32_t sh) const {
    if (empty_ || sw == 0 || sh == 0) return 0.0f;
    auto b = bounds();
    return static_cast<float>(b.width * b.height) /
           static_cast<float>(sw * sh);
}

} // namespace kindle
