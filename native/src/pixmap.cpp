// native/src/pixmap.cpp
#include "kindle/pixmap.hpp"
#include <cassert>
#include <cstring>
#include <algorithm>

namespace kindle {

Pixmap::Pixmap(uint32_t width, uint32_t height)
    : width_(width), height_(height), pixels_(width * height, 0xFF) {}

uint8_t Pixmap::get_pixel(uint32_t x, uint32_t y) const {
    assert(x < width_ && y < height_);
    return pixels_[y * width_ + x];
}

void Pixmap::set_pixel(uint32_t x, uint32_t y, uint8_t value) {
    assert(x < width_ && y < height_);
    pixels_[y * width_ + x] = value;
}

void Pixmap::fill(uint8_t value) {
    std::fill(pixels_.begin(), pixels_.end(), value);
}

void Pixmap::blit(const Pixmap& src, uint32_t dst_x, uint32_t dst_y,
                  const EinkRect& sr) {
    for (uint32_t row = 0; row < sr.height && (dst_y + row) < height_; ++row) {
        for (uint32_t col = 0; col < sr.width && (dst_x + col) < width_; ++col) {
            if ((sr.y + row) < src.height_ && (sr.x + col) < src.width_)
                set_pixel(dst_x + col, dst_y + row,
                          src.get_pixel(sr.x + col, sr.y + row));
        }
    }
}

static uint8_t pack_nibble(uint8_t canvas_pixel) {
    return static_cast<uint8_t>((255u - canvas_pixel) >> 4u);
}

void Pixmap::pack_to_fb0(uint8_t* fb) const {
    uint32_t bytes = (width_ * height_) / 2;
    for (uint32_t i = 0; i < bytes; ++i) {
        uint32_t pixel_idx = i * 2;
        uint8_t l = pack_nibble(pixels_[pixel_idx]);
        uint8_t r = pack_nibble(pixels_[pixel_idx + 1]);
        fb[i] = static_cast<uint8_t>((l << 4u) | r);
    }
}

void Pixmap::pack_region_to_fb0(uint8_t* fb, const EinkRect& region) const {
    uint32_t x0 = region.x & ~1u;  // snap to even
    uint32_t y0 = region.y;
    uint32_t x1 = std::min(x0 + region.width + (region.x & 1u), width_);
    uint32_t y1 = std::min(y0 + region.height, height_);
    uint32_t stride_bytes = width_ / 2;
    for (uint32_t y = y0; y < y1; ++y) {
        for (uint32_t x = x0; x < x1; x += 2) {
            uint8_t l = pack_nibble(pixels_[y * width_ + x]);
            uint8_t r = (x + 1 < x1) ? pack_nibble(pixels_[y * width_ + x + 1]) : 0x0u;
            fb[y * stride_bytes + x / 2] = static_cast<uint8_t>((l << 4u) | r);
        }
    }
}

} // namespace kindle
