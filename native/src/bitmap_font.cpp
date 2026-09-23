// native/src/bitmap_font.cpp
// 8x16 monospace font, sourced from papergram's 5x7 glyph data scaled 2x vertically.
// Character coverage: uppercase A-Z, digits 0-9, basic punctuation, space.
#include "kindle/bitmap_font.hpp"
#include <cctype>
#include <cstring>

namespace kindle {

namespace {

static uint8_t pattern_letter(char upper, int row) {
    static const uint8_t letters[26][7] = {
        {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30},
        {14, 17, 16, 16, 16, 17, 14}, {30, 17, 17, 17, 17, 17, 30},
        {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
        {14, 17, 16, 23, 17, 17, 14}, {17, 17, 17, 31, 17, 17, 17},
        {14,  4,  4,  4,  4,  4, 14}, { 7,  2,  2,  2, 18, 18, 12},
        {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
        {17, 27, 21, 21, 17, 17, 17}, {17, 25, 21, 19, 17, 17, 17},
        {14, 17, 17, 17, 17, 17, 14}, {30, 17, 17, 30, 16, 16, 16},
        {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
        {15, 16, 16, 14,  1,  1, 30}, {31,  4,  4,  4,  4,  4,  4},
        {17, 17, 17, 17, 17, 17, 14}, {17, 17, 17, 17, 17, 10,  4},
        {17, 17, 17, 21, 21, 21, 10}, {17, 17, 10,  4, 10, 17, 17},
        {17, 17, 10,  4,  4,  4,  4}, {31,  1,  2,  4,  8, 16, 31},
    };
    return letters[upper - 'A'][row];
}

static uint8_t pattern_digit(char d, int row) {
    static const uint8_t digits[10][7] = {
        {14, 17, 19, 21, 25, 17, 14}, { 4, 12,  4,  4,  4,  4, 14},
        {14, 17,  1,  2,  4,  8, 31}, {30,  1,  1, 14,  1,  1, 30},
        { 2,  6, 10, 18, 31,  2,  2}, {31, 16, 16, 30,  1,  1, 30},
        {14, 16, 16, 30, 17, 17, 14}, {31,  1,  2,  4,  8,  8,  8},
        {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15,  1,  1, 14},
    };
    return digits[d - '0'][row];
}

static uint8_t pattern_punctuation(char ch, int row) {
    switch (ch) {
        case '.': return row == 6 ? 4 : 0;
        case ',': return row == 6 ? 4 : (row == 5 ? 8 : 0);
        case '-': return row == 3 ? 14 : 0;
        case '+': return row == 3 ? 14 : (row >= 1 && row <= 5 ? 4 : 0);
        case '_': return row == 6 ? 31 : 0;
        case ':': return (row == 2 || row == 5) ? 4 : 0;
        case '!': return row == 6 ? 4 : (row < 5 ? 4 : 0);
        case '?': {
            if (row == 0) return 14;
            if (row == 1) return 17;
            if (row == 2) return 1;
            if (row == 3) return 2;
            if (row == 4 || row == 6) return 4;
            return 0;
        }
        case '[': return (row == 0 || row == 6) ? 14 : 8;
        case ']': return (row == 0 || row == 6) ? 14 : 2;
        case '(': return (row == 0 || row == 6) ? 4 : 8;
        case ')': return (row == 0 || row == 6) ? 4 : 2;
        case '/': return row < 2 ? 2 : (row < 5 ? 4 : 8);
        case '%': return (row == 0 || row == 5) ? 10 : 4;
        default: return 0;
    }
}

// Returns the 5-bit row pattern for the given character and 5x7 row index (0-6).
static uint8_t pattern_row(char c, int row) {
    unsigned char value = static_cast<unsigned char>(c);
    // Normalize to uppercase
    char upper = c;
    if (value >= 'a' && value <= 'z') upper = static_cast<char>(value - ('a' - 'A'));
    if (value < 32 || value > 126) upper = '?';

    if (upper >= 'A' && upper <= 'Z') return pattern_letter(upper, row);
    if (upper >= '0' && upper <= '9') return pattern_digit(upper, row);
    if (upper == ' ') return 0;
    return pattern_punctuation(upper, row);
}

} // namespace

void BitmapFont::draw_char(Pixmap& canvas, uint32_t x, uint32_t y, char c,
                            uint8_t fg, uint8_t bg) const {
    // 8x16: rows 0 and 15 are blank borders; rows 1-14 map from 5x7 data (each row scaled 2x)
    for (uint32_t row = 0; row < GLYPH_H; ++row) {
        uint8_t bits = 0;
        if (row > 0 && row < GLYPH_H - 1) {
            int src_row = static_cast<int>((row - 1) / 2);
            bits = static_cast<uint8_t>(pattern_row(c, src_row) << 1u);  // left-justify in 8 bits
        }
        for (uint32_t col = 0; col < GLYPH_W; ++col) {
            if ((x + col) < canvas.width() && (y + row) < canvas.height()) {
                uint8_t mask = static_cast<uint8_t>(1u << (GLYPH_W - 1u - col));
                canvas.set_pixel(x + col, y + row, (bits & mask) ? fg : bg);
            }
        }
    }
}

void BitmapFont::draw_string(Pixmap& canvas, uint32_t x, uint32_t y,
                              const char* s, uint8_t fg, uint8_t bg) const {
    while (s && *s) {
        if (x + GLYPH_W > canvas.width()) break;
        draw_char(canvas, x, y, *s++, fg, bg);
        x += GLYPH_W;
    }
}

} // namespace kindle
