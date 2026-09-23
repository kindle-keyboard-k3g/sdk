// native/tests/test_bitmap_font.cpp
#include "kindle/bitmap_font.hpp"
#include "kindle/pixmap.hpp"
#include <cassert>
#include <iostream>

int main() {
    kindle::BitmapFont font;
    kindle::Pixmap canvas(80, 16);

    // Drawing a character changes at least one pixel in the glyph area
    font.draw_char(canvas, 0, 0, 'A', 0x00, 0xFF);
    bool changed = false;
    for (uint32_t x = 0; x < kindle::BitmapFont::GLYPH_W && !changed; ++x)
        changed = (canvas.get_pixel(x, 0) == 0x00);  // black pixel found
    // Note: row 0 is blank (border), check inner rows
    if (!changed) {
        for (uint32_t row = 0; row < kindle::BitmapFont::GLYPH_H && !changed; ++row)
            for (uint32_t x = 0; x < kindle::BitmapFont::GLYPH_W && !changed; ++x)
                changed = (canvas.get_pixel(x, row) == 0x00);
    }
    assert(changed);

    // Background pixels outside glyph are bg color
    kindle::Pixmap canvas2(8, 16);
    canvas2.fill(0x80);  // gray baseline
    font.draw_char(canvas2, 0, 0, ' ', 0x00, 0xFF);  // space: all bg
    for (uint32_t x = 0; x < 8; ++x)
        assert(canvas2.get_pixel(x, 0) == 0xFF);  // bg replaced

    // draw_string renders multiple glyphs side by side
    kindle::Pixmap wide(80, 16);
    font.draw_string(wide, 0, 0, "OK", 0x00, 0xFF);
    // second glyph area starts at x=8
    bool second_glyph = false;
    for (uint32_t row = 0; row < 16 && !second_glyph; ++row)
        for (uint32_t x = 8; x < 16 && !second_glyph; ++x)
            second_glyph = (wide.get_pixel(x, row) == 0x00);
    assert(second_glyph);

    std::cout << "PASS: test_bitmap_font\n";
    return 0;
}
