// native/tests/test_dirty.cpp
#include "kindle/dirty.hpp"
#include "kindle/eink.hpp"
#include <cassert>
#include <iostream>

int main() {
    kindle::DirtyRegion d;
    assert(d.empty());

    // Mark a rect, get bounds back
    d.mark({10, 20, 30, 40});
    assert(!d.empty());
    auto b = d.bounds();
    assert(b.x == 10 && b.y == 20 && b.width == 30 && b.height == 40);

    // Union with another rect
    d.mark({50, 10, 10, 20});
    b = d.bounds();
    assert(b.x == 10 && b.y == 10);
    assert(b.x + b.width == 60);
    assert(b.y + b.height == 60);

    // Odd x must snap to even
    kindle::DirtyRegion d2;
    d2.mark({3, 0, 2, 2});  // x=3 -> snap to 2, width grows by 1
    assert(d2.bounds().x == 2);
    assert(d2.bounds().x % 2 == 0);

    // clear
    d.clear();
    assert(d.empty());

    // area_ratio
    kindle::DirtyRegion d3;
    d3.mark_all(600, 800);
    assert(d3.area_ratio(600, 800) == 1.0f);

    d3.clear();
    d3.mark({0, 0, 300, 400});
    float ratio = d3.area_ratio(600, 800);
    assert(ratio > 0.24f && ratio < 0.26f);

    std::cout << "PASS: test_dirty\n";
    return 0;
}
