// native/tests/test_frame_differ.cpp
#include "kindle/frame_differ.hpp"
#include "kindle/pixmap.hpp"
#include <cassert>
#include <iostream>

int main() {
    kindle::Pixmap a(8, 4), b(8, 4);

    // Identical frames -> empty region
    auto r = kindle::FrameDiffer::compute(a, b, 0);
    assert(r.width == 0 && r.height == 0);

    // Change one pixel
    b.set_pixel(4, 2, 0x00);
    r = kindle::FrameDiffer::compute(a, b, 0);
    assert(r.x <= 4 && r.y <= 2);
    assert(r.x + r.width  >= 5);
    assert(r.y + r.height >= 3);
    assert(r.x % 2 == 0);  // nibble-aligned

    // Ghosting margin expands the rect
    auto r2 = kindle::FrameDiffer::compute(a, b, 2);
    assert(r2.width  >= r.width  + 4 || r2.x < r.x);
    assert(r2.height >= r.height + 4 || r2.y < r.y);

    std::cout << "PASS: test_frame_differ\n";
    return 0;
}
