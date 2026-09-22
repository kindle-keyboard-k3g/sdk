#include "kindle/input.hpp"
#include <cassert>
#include <iostream>
#include <linux/input.h>

#include "../platform/linux/evdev_linux.cpp"

int main() {
    std::cout << "Running test_input..." << std::endl;

    kindle::FakeInputDevice input;
    assert(input.open("/dev/fake_input"));

    // Inject D-Pad Up Press
    input.inject_raw_event(KEY_UP, 1);
    // Inject D-Pad Up Release
    input.inject_raw_event(KEY_UP, 0);

    // Inject Select Press
    input.inject_raw_event(KEY_ENTER, 1);

    // Inject PageDown Press
    input.inject_raw_event(KEY_PAGEDOWN, 1);

    kindle::InputEvent ev;

    assert(input.poll_event(ev));
    assert(ev.key == kindle::KeyCode::Up);
    assert(ev.type == kindle::KeyEventType::Press);

    assert(input.poll_event(ev));
    assert(ev.key == kindle::KeyCode::Up);
    assert(ev.type == kindle::KeyEventType::Release);

    assert(input.poll_event(ev));
    assert(ev.key == kindle::KeyCode::Select);
    assert(ev.type == kindle::KeyEventType::Press);

    assert(input.poll_event(ev));
    assert(ev.key == kindle::KeyCode::PageDown);
    assert(ev.type == kindle::KeyEventType::Press);

    assert(!input.poll_event(ev));

    input.close();
    std::cout << "PASS: test_input verified successfully!" << std::endl;
    return 0;
}
