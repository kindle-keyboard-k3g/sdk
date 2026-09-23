// native/src/ghosting.cpp
#include "kindle/ghosting.hpp"
#include <chrono>

namespace kindle {

DirtyRegion DarkToWhiteCleaner::detect(const Pixmap& prev,
                                        const Pixmap& cur) const {
    DirtyRegion d;
    uint32_t w = prev.width(), h = prev.height();
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            uint8_t p = prev.get_pixel(x, y);
            uint8_t c = cur.get_pixel(x, y);
            if (p <= 0x7F && c >= 0xF0)  // dark -> white
                d.mark(x, y, 1, 1);
        }
    }
    return d;
}

IdleRefreshScheduler::IdleRefreshScheduler(uint32_t idle_ms,
                                             uint32_t tile_w,
                                             uint32_t tile_h)
    : idle_ms_(idle_ms), tile_w_(tile_w), tile_h_(tile_h) {}

void IdleRefreshScheduler::notify_activity() {
    last_activity_ = Clock::now();
    tile_index_ = 0;
}

bool IdleRefreshScheduler::is_active() const {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now() - last_activity_).count();
    return static_cast<uint32_t>(elapsed) >= idle_ms_;
}

EinkRect IdleRefreshScheduler::next_tile(uint32_t sw, uint32_t sh) {
    if (!is_active()) return {0, 0, 0, 0};
    uint32_t cols = (sw + tile_w_ - 1) / tile_w_;
    uint32_t rows = (sh + tile_h_ - 1) / tile_h_;
    uint32_t total = cols * rows;
    if (tile_index_ >= total) {
        tile_index_ = 0;  // wrap: restart sweep
        return {0, 0, 0, 0};
    }
    uint32_t col = tile_index_ % cols;
    uint32_t row = tile_index_ / cols;
    ++tile_index_;
    uint32_t x = col * tile_w_;
    uint32_t y = row * tile_h_;
    uint32_t w = (x + tile_w_ <= sw) ? tile_w_ : (sw - x);
    uint32_t h = (y + tile_h_ <= sh) ? tile_h_ : (sh - y);
    return {x, y, w, h};
}

void IdleRefreshScheduler::reset() {
    notify_activity();
}

} // namespace kindle
