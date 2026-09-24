#include "kindle/input.hpp"
#include "kindle/input_debouncer.hpp"
#include <cassert>
#include <iostream>

void test_debouncer_bounce() {
    std::cout << "Testing InputDebouncer bounce suppression...\n";
    kindle::DebounceConfig config;
    config.debounce_ms = 20;
    kindle::InputDebouncer debouncer(config);

    kindle::InputEvent press_ev{kindle::KeyCode::Space, kindle::KeyEventType::Press, 57, 0};
    kindle::InputEvent release_ev{kindle::KeyCode::Space, kindle::KeyEventType::Release, 57, 0};

    // Initial press at t=100ms -> accepted
    assert(debouncer.filter(press_ev, 100));

    // Rapid release at t=110ms (10ms < 20ms debounce) -> suppressed as bounce
    assert(!debouncer.filter(release_ev, 110));

    // Release at t=130ms (30ms > 20ms debounce) -> accepted
    assert(debouncer.filter(release_ev, 130));

    // Rapid press at t=140ms (10ms < 20ms debounce) -> suppressed as bounce
    assert(!debouncer.filter(press_ev, 140));

    // Press at t=160ms -> accepted
    assert(debouncer.filter(press_ev, 160));

    std::cout << "PASS: test_debouncer_bounce\n";
}

void test_debouncer_repeat_disabled() {
    std::cout << "Testing InputDebouncer repeat disabled...\n";
    kindle::DebounceConfig config;
    config.repeat_enabled = false;
    kindle::InputDebouncer debouncer(config);

    kindle::InputEvent press_ev{kindle::KeyCode::Up, kindle::KeyEventType::Press, 103, 0};
    kindle::InputEvent repeat_ev{kindle::KeyCode::Up, kindle::KeyEventType::Repeat, 103, 0};

    assert(debouncer.filter(press_ev, 100));
    assert(!debouncer.filter(repeat_ev, 200));
    assert(!debouncer.filter(repeat_ev, 600));

    std::cout << "PASS: test_debouncer_repeat_disabled\n";
}

void test_debouncer_repeat_timing() {
    std::cout << "Testing InputDebouncer repeat timing...\n";
    kindle::DebounceConfig config;
    config.repeat_enabled = true;
    config.repeat_delay_ms = 400;
    config.repeat_interval_ms = 80;
    kindle::InputDebouncer debouncer(config);

    kindle::InputEvent press_ev{kindle::KeyCode::A, kindle::KeyEventType::Press, 30, 0};
    kindle::InputEvent repeat_ev{kindle::KeyCode::A, kindle::KeyEventType::Repeat, 30, 0};

    // Press at t=0
    assert(debouncer.filter(press_ev, 0));

    // Repeat before repeat_delay_ms (400ms)
    assert(!debouncer.filter(repeat_ev, 100));
    assert(!debouncer.filter(repeat_ev, 399));

    // First repeat at t=400ms -> accepted
    assert(debouncer.filter(repeat_ev, 400));

    // Repeat before interval (80ms)
    assert(!debouncer.filter(repeat_ev, 450));

    // Repeat after interval (400 + 80 = 480ms) -> accepted
    assert(debouncer.filter(repeat_ev, 480));

    // Another repeat at t=560ms -> accepted
    assert(debouncer.filter(repeat_ev, 560));

    std::cout << "PASS: test_debouncer_repeat_timing\n";
}

int main() {
    test_debouncer_bounce();
    test_debouncer_repeat_disabled();
    test_debouncer_repeat_timing();
    std::cout << "All InputDebouncer tests passed.\n";
    return 0;
}
