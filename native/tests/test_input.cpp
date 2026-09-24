#include "kindle/input.hpp"
#include "kindle/key_catalog.hpp"
#include "kindle/fake_input.hpp"
#include "kindle/linux_input.hpp"
#include <cassert>
#include <iostream>
#include <linux/input.h>

void test_key_catalog() {
    std::cout << "Testing KeyCatalog and extended KeyCodes...\n";

    // 1. Verify Alphanumeric KeyCodes
    assert(kindle::KeyCatalog::map_scancode(16, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Q);
    assert(kindle::KeyCatalog::map_scancode(30, kindle::DeviceModel::Kindle3) == kindle::KeyCode::A);
    assert(kindle::KeyCatalog::map_scancode(44, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Z);
    assert(kindle::KeyCatalog::map_scancode(2, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Num1);
    assert(kindle::KeyCatalog::map_scancode(11, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Num0);

    // 2. Verify Punctuation & Editing
    assert(kindle::KeyCatalog::map_scancode(28, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Enter);
    assert(kindle::KeyCatalog::map_scancode(57, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Space);
    assert(kindle::KeyCatalog::map_scancode(14, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Backspace);
    assert(kindle::KeyCatalog::map_scancode(52, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Dot);
    assert(kindle::KeyCatalog::map_scancode(53, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Slash);
    assert(kindle::KeyCatalog::map_scancode(51, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Comma);
    assert(kindle::KeyCatalog::map_scancode(39, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Semicolon);
    assert(kindle::KeyCatalog::map_scancode(40, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Apostrophe);
    assert(kindle::KeyCatalog::map_scancode(12, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Minus);
    assert(kindle::KeyCatalog::map_scancode(13, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Equal);
    assert(kindle::KeyCatalog::map_scancode(26, kindle::DeviceModel::Kindle3) == kindle::KeyCode::LeftBracket);
    assert(kindle::KeyCatalog::map_scancode(27, kindle::DeviceModel::Kindle3) == kindle::KeyCode::RightBracket);
    assert(kindle::KeyCatalog::map_scancode(43, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Backslash);

    // 3. Verify Modifiers
    assert(kindle::KeyCatalog::map_scancode(42, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Shift);
    assert(kindle::KeyCatalog::map_scancode(54, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Shift);
    assert(kindle::KeyCatalog::map_scancode(56, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Alt);
    assert(kindle::KeyCatalog::map_scancode(100, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Alt);

    // 4. Verify K3 vs DX Hardware Scancodes
    assert(kindle::KeyCatalog::map_scancode(158, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Back);
    assert(kindle::KeyCatalog::map_scancode(91, kindle::DeviceModel::KindleDX) == kindle::KeyCode::Back);

    assert(kindle::KeyCatalog::map_scancode(102, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Home);
    assert(kindle::KeyCatalog::map_scancode(98, kindle::DeviceModel::KindleDX) == kindle::KeyCode::Home);

    assert(kindle::KeyCatalog::map_scancode(194, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Select);
    assert(kindle::KeyCatalog::map_scancode(92, kindle::DeviceModel::KindleDX) == kindle::KeyCode::Select);

    assert(kindle::KeyCatalog::map_scancode(103, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Up);
    assert(kindle::KeyCatalog::map_scancode(122, kindle::DeviceModel::KindleDX) == kindle::KeyCode::Up);

    assert(kindle::KeyCatalog::map_scancode(108, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Down);
    assert(kindle::KeyCatalog::map_scancode(123, kindle::DeviceModel::KindleDX) == kindle::KeyCode::Down);

    assert(kindle::KeyCatalog::map_scancode(126, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Sym);
    assert(kindle::KeyCatalog::map_scancode(94, kindle::DeviceModel::KindleDX) == kindle::KeyCode::Sym);

    assert(kindle::KeyCatalog::map_scancode(190, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Ctrl);
    assert(kindle::KeyCatalog::map_scancode(90, kindle::DeviceModel::KindleDX) == kindle::KeyCode::Ctrl);

    assert(kindle::KeyCatalog::map_scancode(115, kindle::DeviceModel::Kindle3) == kindle::KeyCode::VolumeUp);
    assert(kindle::KeyCatalog::map_scancode(114, kindle::DeviceModel::Kindle3) == kindle::KeyCode::VolumeDown);
    assert(kindle::KeyCatalog::map_scancode(116, kindle::DeviceModel::Kindle3) == kindle::KeyCode::Power);

    std::cout << "PASS: test_key_catalog\n";
}

void test_fake_input_multi() {
    std::cout << "Testing FakeInputDevice...\n";
    kindle::FakeInputDevice fake;
    assert(fake.open("/dev/fake_input"));
    assert(!fake.is_grabbed());

    // Test grab / release simulation
    fake.grab();
    assert(fake.is_grabbed());
    fake.release();
    assert(!fake.is_grabbed());

    // Inject sequence from multiple simulated device nodes
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_A, 1);    // Event0: Keyboard A
    fake.inject_raw_event(kindle::KeyCatalog::CODE_FW_UP_K3, 1);  // Event1: D-pad Up
    fake.inject_raw_event(kindle::KeyCatalog::CODE_PG_FWD_K3, 1); // Event2: Page rocker

    kindle::InputEvent ev;
    assert(fake.poll_event(ev));
    assert(ev.key == kindle::KeyCode::A && ev.type == kindle::KeyEventType::Press);

    assert(fake.poll_event(ev));
    assert(ev.key == kindle::KeyCode::Up && ev.type == kindle::KeyEventType::Press);

    assert(fake.poll_event(ev));
    assert(ev.key == kindle::KeyCode::PageDown && ev.type == kindle::KeyEventType::Press);

    assert(!fake.poll_event(ev));

    // Test inject_key directly
    fake.inject_key(kindle::KeyCode::Home, kindle::KeyEventType::Press);
    assert(fake.poll_event(ev));
    assert(ev.key == kindle::KeyCode::Home && ev.type == kindle::KeyEventType::Press);
    ev.timestamp_us = 123456ULL;
    assert(ev.timestamp_us == 123456ULL);

    fake.close();
    std::cout << "PASS: test_fake_input_multi\n";
}

void test_linux_input_instantiation() {
    std::cout << "Testing LinuxEvdevInputDevice instantiation & grab state...\n";
    kindle::LinuxEvdevInputDevice dev({"/dev/null"}, false, kindle::DeviceModel::Kindle3);
    assert(!dev.is_grabbed());
    dev.close();
    std::cout << "PASS: test_linux_input_instantiation\n";
}

int main() {
    std::cout << "Running test_input..." << std::endl;
    test_key_catalog();
    test_fake_input_multi();
    test_linux_input_instantiation();
    std::cout << "PASS: test_input verified successfully!" << std::endl;
    return 0;
}
