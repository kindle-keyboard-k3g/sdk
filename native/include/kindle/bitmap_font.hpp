// native/include/kindle/bitmap_font.hpp
#pragma once
#include "kindle/pixmap.hpp"
#include <cstdint>

namespace kindle {

class BitmapFont {
public:
    static constexpr uint32_t GLYPH_W = 8;
    static constexpr uint32_t GLYPH_H = 16;

    // Draw character c at (x, y). Unsupported chars render as '?'.
    void draw_char(Pixmap& canvas, uint32_t x, uint32_t y, char c,
                   uint8_t fg = 0x00, uint8_t bg = 0xFF) const;

    // Draw null-terminated string; glyphs advance by GLYPH_W.
    void draw_string(Pixmap& canvas, uint32_t x, uint32_t y, const char* s,
                     uint8_t fg = 0x00, uint8_t bg = 0xFF) const;
};

} // namespace kindle
