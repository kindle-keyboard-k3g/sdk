// native/tests/test_pixmap.cpp
#include "kindle/pixmap.hpp"
#include "kindle/eink.hpp"
#include <cassert>
#include <iostream>
#include <vector>

int main() {
    // Construction: 8bpp, all white by default
    kindle::Pixmap p(4, 2);
    assert(p.width() == 4);
    assert(p.height() == 2);
    assert(p.stride() == 4);
    assert(p.get_pixel(0, 0) == 0xFF);

    // set_pixel / get_pixel
    p.set_pixel(2, 1, 0x00);
    assert(p.get_pixel(2, 1) == 0x00);
    assert(p.get_pixel(3, 1) == 0xFF);  // neighbor unchanged

    // fill
    p.fill(0x80);
    assert(p.get_pixel(0, 0) == 0x80);

    // pack_to_fb0: K3 Pearl nibble = (255 - pixel) >> 4
    // black(0x00)->nibble 0xF, white(0xFF)->nibble 0x0
    // Byte = (left_nibble << 4) | right_nibble
    kindle::Pixmap mono(2, 1);
    mono.set_pixel(0, 0, 0x00);  // black left  -> nibble 0xF
    mono.set_pixel(1, 0, 0xFF);  // white right -> nibble 0x0
    std::vector<uint8_t> fb(1, 0xAA);
    mono.pack_to_fb0(fb.data());
    assert(fb[0] == 0xF0);  // black left, white right

    // pack_region_to_fb0: only updates covered bytes
    kindle::Pixmap p2(4, 2);
    p2.fill(0x00);  // all black -> nibble 0xF -> bytes 0xFF
    std::vector<uint8_t> fb2(4, 0x00);
    kindle::EinkRect region{0, 0, 4, 2};
    p2.pack_region_to_fb0(fb2.data(), region);
    assert(fb2[0] == 0xFF);
    assert(fb2[3] == 0xFF);

    std::cout << "PASS: test_pixmap\n";
    return 0;
}
