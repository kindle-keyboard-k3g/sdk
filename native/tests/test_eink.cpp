#include "kindle/eink.hpp"
#include <cassert>
#include <iostream>

#include "../platform/fake/fake_eink.cpp"

int main() {
    std::cout << "Running test_eink..." << std::endl;

    kindle::FakeEinkDisplay display(600, 800, kindle::PixelFormat::Gray4);
    assert(display.get_width() == 600);
    assert(display.get_height() == 800);
    assert(display.get_format() == kindle::PixelFormat::Gray4);

    assert(display.open());
    assert(display.get_framebuffer() != nullptr);

    // Initial white screen
    assert(display.get_framebuffer()[0] == 0xFF);

    // Update screen
    kindle::EinkRect rect{0, 0, 600, 800};
    assert(display.update(kindle::RefreshMode::Full, rect));
    assert(display.get_update_count() == 1);
    assert(display.get_last_mode() == kindle::RefreshMode::Full);

    display.clear_screen();
    assert(display.get_update_count() == 2);

    display.close();
    std::cout << "PASS: test_eink verified!" << std::endl;
    return 0;
}
