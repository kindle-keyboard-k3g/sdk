// native/include/kindle/dirty.hpp
#pragma once
#include "kindle/eink.hpp"
#include <cstdint>

namespace kindle {

class DirtyRegion {
public:
    void mark(const EinkRect& rect);
    void mark(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    void mark_all(uint32_t screen_w, uint32_t screen_h);
    void expand(uint32_t margin, uint32_t screen_w, uint32_t screen_h);

    EinkRect bounds() const;
    bool     empty()  const { return empty_; }
    void     clear()        { empty_ = true; }

    // Ratio of dirty area to total (0.0 - 1.0)
    float area_ratio(uint32_t screen_w, uint32_t screen_h) const;

private:
    bool     empty_ = true;
    uint32_t x0_ = 0, y0_ = 0, x1_ = 0, y1_ = 0;

    // Snaps x to even (nibble boundary)
    static uint32_t align_x(uint32_t x) { return x & ~1u; }
};

} // namespace kindle
