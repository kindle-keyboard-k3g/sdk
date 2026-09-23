// native/tests/test_ghosting.cpp
#include "kindle/ghosting.hpp"
#include "kindle/pixmap.hpp"
#include <cassert>
#include <iostream>

int main() {
    // DarkToWhiteCleaner: detect dark->white transitions
    kindle::DarkToWhiteCleaner cleaner;
    kindle::Pixmap prev(4, 2), cur(4, 2);

    // No change -> empty
    auto d = cleaner.detect(prev, cur);
    assert(d.empty());

    // White->black transition (no cleanup needed)
    cur.set_pixel(0, 0, 0x00);
    d = cleaner.detect(prev, cur);
    assert(d.empty());

    // Dark->white transition: prev black, cur white
    kindle::Pixmap prev2(4, 2), cur2(4, 2);
    prev2.set_pixel(2, 1, 0x00);  // dark pixel in prev
    // cur2 is all white (default)
    d = cleaner.detect(prev2, cur2);
    assert(!d.empty());
    assert(d.bounds().x <= 2 && d.bounds().y <= 1);

    // IdleRefreshScheduler: not active before idle timeout
    kindle::IdleRefreshScheduler sched(50000, 150, 200);  // 50s idle
    assert(!sched.is_active());
    auto tile = sched.next_tile(600, 800);
    assert(tile.width == 0);  // not active yet

    // After notify_activity, reset timer
    sched.notify_activity();
    assert(!sched.is_active());

    std::cout << "PASS: test_ghosting\n";
    return 0;
}
