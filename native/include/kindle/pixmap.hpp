// native/include/kindle/pixmap.hpp
#pragma once
#include "kindle/eink.hpp"
#include <cstdint>
#include <vector>

namespace kindle {

class Pixmap {
public:
    Pixmap(uint32_t width, uint32_t height);

    uint8_t  get_pixel(uint32_t x, uint32_t y) const;
    void     set_pixel(uint32_t x, uint32_t y, uint8_t value);
    void     fill(uint8_t value = 0xFF);
    void     blit(const Pixmap& src, uint32_t dst_x, uint32_t dst_y,
                  const EinkRect& src_rect);

    // Packs 8bpp canvas to 4bpp fb0. nibble = (255 - pixel) >> 4.
    // Byte layout: (left_nibble << 4) | right_nibble.
    void pack_to_fb0(uint8_t* fb) const;
    void pack_region_to_fb0(uint8_t* fb, const EinkRect& region) const;

    uint32_t        width()  const { return width_; }
    uint32_t        height() const { return height_; }
    uint32_t        stride() const { return width_; }
    uint8_t*        data()         { return pixels_.data(); }
    const uint8_t*  data()   const { return pixels_.data(); }

private:
    uint32_t              width_;
    uint32_t              height_;
    std::vector<uint8_t>  pixels_;
};

} // namespace kindle
