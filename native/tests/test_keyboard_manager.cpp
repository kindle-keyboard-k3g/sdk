#include "kindle/input.hpp"
#include "kindle/fake_input.hpp"
#include "kindle/input_debouncer.hpp"
#include "kindle/modifier_tracker.hpp"
#include "kindle/shortcut_registry.hpp"
#include "kindle/keyboard_manager.hpp"
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

void test_modifier_tracker_disabled() {
    std::cout << "Testing ModifierTracker with LatchMode::Disabled...\n";
    kindle::ModifierTracker tracker(kindle::LatchMode::Disabled);

    assert(!tracker.state().shift);
    assert(!tracker.state().alt);
    assert(!tracker.state().ctrl);
    assert(!tracker.state().sym);

    // Press Shift
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Press);
    assert(tracker.state().shift);
    assert(!tracker.is_latched());

    // Release Shift
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Release);
    assert(!tracker.state().shift);
    assert(!tracker.is_latched());

    std::cout << "PASS: test_modifier_tracker_disabled\n";
}

void test_modifier_tracker_sticky_once() {
    std::cout << "Testing ModifierTracker with LatchMode::StickyOnce...\n";
    kindle::ModifierTracker tracker(kindle::LatchMode::StickyOnce);

    // Tap and release Shift -> latched
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Press);
    assert(tracker.state().shift);
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Release);
    assert(tracker.state().shift); // still true because latched
    assert(tracker.is_latched());

    // Non-modifier key is pressed, latch is consumed
    tracker.consume_latch();
    assert(!tracker.state().shift);
    assert(!tracker.is_latched());

    // Physical hold: hold Shift, consume latch, release Shift -> should not latch
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Press);
    assert(tracker.state().shift);
    tracker.consume_latch(); // key typed while held
    assert(tracker.state().shift); // still true because physically held!
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Release);
    assert(!tracker.state().shift);
    assert(!tracker.is_latched());

    std::cout << "PASS: test_modifier_tracker_sticky_once\n";
}

void test_shortcut_registry() {
    std::cout << "Testing ShortcutRegistry pre-bound shortcuts and custom callbacks...\n";
    kindle::ShortcutRegistry registry;

    kindle::ModifierState alt_mods{};
    alt_mods.alt = true;

    kindle::ModifierState no_mods{};

    // Alt+G triggers Ghostbuster
    assert(registry.check(kindle::KeyCode::G, alt_mods) == kindle::ShortcutAction::Ghostbuster);
    assert(registry.check(kindle::KeyCode::G, no_mods) == kindle::ShortcutAction::None);

    // Volume keys
    assert(registry.check(kindle::KeyCode::VolumeUp, no_mods) == kindle::ShortcutAction::VolumeUp);
    assert(registry.check(kindle::KeyCode::VolumeDown, no_mods) == kindle::ShortcutAction::VolumeDown);

    // Custom callback test
    int callback_invocations = 0;
    auto handler = [](void* user_data) {
        auto* count = static_cast<int*>(user_data);
        ++(*count);
    };

    kindle::ModifierState ctrl_mods{};
    ctrl_mods.ctrl = true;

    assert(registry.bind_callback(ctrl_mods.to_mask(), kindle::KeyCode::Q, handler, &callback_invocations));
    assert(registry.check(kindle::KeyCode::Q, ctrl_mods) == kindle::ShortcutAction::None);
    assert(callback_invocations == 1);

    // Re-check with no mods -> handler not called
    registry.check(kindle::KeyCode::Q, no_mods);
    assert(callback_invocations == 1);

    std::cout << "PASS: test_shortcut_registry\n";
}

void test_keyboard_manager_pipeline() {
    std::cout << "Testing KeyboardManager unified pipeline...\n";
    kindle::FakeInputDevice fake;
    assert(fake.open());

    kindle::DebounceConfig deb_cfg;
    deb_cfg.debounce_ms = 0; // immediate for testing
    kindle::KeyboardManager mgr(fake, kindle::DeviceModel::Kindle3, deb_cfg, kindle::LatchMode::Disabled);

    // 1. Regular character typing
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_A, 1); // Press 'A'
    kindle::KeyEvent ev;
    assert(mgr.poll(ev));
    assert(ev.key == kindle::KeyCode::A);
    assert(ev.type == kindle::KeyEventType::Press);
    assert(ev.text == "a");
    assert(!ev.is_shortcut);

    // 2. Alt+G Ghostbuster shortcut
    fake.inject_raw_event(kindle::KeyCatalog::CODE_ALT_L, 1); // Press Alt
    assert(mgr.poll(ev));
    assert(ev.key == kindle::KeyCode::Alt);

    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_G, 1); // Press G
    assert(mgr.poll(ev));
    assert(ev.key == kindle::KeyCode::G);
    assert(ev.is_shortcut);
    assert(ev.shortcut_action == kindle::ShortcutAction::Ghostbuster);
    assert(ev.text.empty());

    fake.inject_raw_event(kindle::KeyCatalog::CODE_ALT_L, 0); // Release Alt
    assert(mgr.poll(ev));
    assert(ev.key == kindle::KeyCode::Alt);

    // 3. StickyOnce mode test
    kindle::KeyboardManager sticky_mgr(fake, kindle::DeviceModel::Kindle3, deb_cfg, kindle::LatchMode::StickyOnce);

    fake.inject_raw_event(kindle::KeyCatalog::CODE_SHIFT_L, 1); // Press Shift
    assert(sticky_mgr.poll(ev));
    assert(ev.key == kindle::KeyCode::Shift);

    fake.inject_raw_event(kindle::KeyCatalog::CODE_SHIFT_L, 0); // Release Shift -> now latched!
    assert(sticky_mgr.poll(ev));
    assert(ev.key == kindle::KeyCode::Shift);
    assert(sticky_mgr.modifiers().shift);

    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_A, 1); // Press 'a' -> should be uppercase 'A'
    assert(sticky_mgr.poll(ev));
    assert(ev.key == kindle::KeyCode::A);
    assert(ev.text == "A");

    // Next key should be lowercase because latch was consumed
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_B, 1); // Press 'b'
    assert(sticky_mgr.poll(ev));
    assert(ev.key == kindle::KeyCode::B);
    assert(ev.text == "b");

    fake.close();
    std::cout << "PASS: test_keyboard_manager_pipeline\n";
}

void test_consumer_recipe_dino() {
    std::cout << "Testing Consumer Recipe: dino (Game sound and jump/duck control)...\n";
    kindle::FakeInputDevice fake;
    assert(fake.open());

    // Dino recipe: repeat disabled, latch disabled for minimum latency
    kindle::DebounceConfig deb_cfg;
    deb_cfg.repeat_enabled = false;
    deb_cfg.debounce_ms = 0;
    kindle::KeyboardManager mgr(fake, kindle::DeviceModel::Kindle3, deb_cfg, kindle::LatchMode::Disabled);

    bool jump_called = false;
    bool duck_called = false;
    uint8_t volume = 50;

    // Simulate game event loop reading from KeyboardManager
    fake.inject_raw_event(kindle::KeyCatalog::CODE_SPACE, 1); // Jump with Space
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_D, 1); // Duck press with D
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_D, 0); // Duck release
    fake.inject_raw_event(kindle::KeyCatalog::CODE_VOL_UP, 1); // VolumeUp

    kindle::KeyEvent ev;
    // 1. Jump
    assert(mgr.poll(ev));
    if (ev.key == kindle::KeyCode::Space && ev.type == kindle::KeyEventType::Press) {
        jump_called = true;
    }
    assert(jump_called);

    // 2. Duck Press
    assert(mgr.poll(ev));
    if (ev.key == kindle::KeyCode::D && ev.type == kindle::KeyEventType::Press) {
        duck_called = true;
    }
    assert(duck_called);

    // 3. Duck Release
    assert(mgr.poll(ev));
    if (ev.key == kindle::KeyCode::D && ev.type == kindle::KeyEventType::Release) {
        duck_called = false;
    }
    assert(!duck_called);

    // 4. Volume Up
    assert(mgr.poll(ev));
    if (ev.shortcut_action == kindle::ShortcutAction::VolumeUp) {
        volume = (volume <= 90) ? volume + 10 : 100;
    }
    assert(volume == 60);

    fake.close();
    std::cout << "PASS: test_consumer_recipe_dino\n";
}

void test_consumer_recipe_papergram() {
    std::cout << "Testing Consumer Recipe: papergram (Messaging text input & Alt+G)...\n";
    kindle::FakeInputDevice fake;
    assert(fake.open());

    // Papergram recipe: StickyOnce for handheld one-thumb typing
    kindle::DebounceConfig deb_cfg;
    deb_cfg.debounce_ms = 0;
    kindle::KeyboardManager mgr(fake, kindle::DeviceModel::Kindle3, deb_cfg, kindle::LatchMode::StickyOnce);

    std::string message_buffer;
    bool ghostbuster_triggered = false;

    // Type "Hi," -> Shift tap, H, i, Alt+Dot (comma on K3)
    fake.inject_raw_event(kindle::KeyCatalog::CODE_SHIFT_L, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_SHIFT_L, 0);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_H, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_I, 1);
    // Alt + Dot -> Comma
    fake.inject_raw_event(kindle::KeyCatalog::CODE_ALT_L, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_DOT, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_ALT_L, 0);
    // Alt + G -> Full refresh
    fake.inject_raw_event(kindle::KeyCatalog::CODE_ALT_L, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_G, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_ALT_L, 0);

    kindle::KeyEvent ev;
    while (mgr.poll(ev)) {
        if (ev.shortcut_action == kindle::ShortcutAction::Ghostbuster) {
            ghostbuster_triggered = true;
        } else if (!ev.text.empty() && ev.type == kindle::KeyEventType::Press) {
            message_buffer.append(ev.text);
        }
    }

    assert(message_buffer == "Hi,");
    assert(ghostbuster_triggered);

    fake.close();
    std::cout << "PASS: test_consumer_recipe_papergram\n";
}

void test_consumer_recipe_kindle_myts() {
    std::cout << "Testing Consumer Recipe: kindle-myts (Terminal ANSI escapes & Ctrl chords)...\n";
    kindle::FakeInputDevice fake;
    assert(fake.open());

    kindle::DebounceConfig deb_cfg;
    deb_cfg.debounce_ms = 0;
    kindle::KeyboardManager mgr(fake, kindle::DeviceModel::Kindle3, deb_cfg, kindle::LatchMode::Disabled);

    std::string pty_stream;

    // 1. Up arrow -> \033[A
    fake.inject_raw_event(kindle::KeyCatalog::CODE_FW_UP_K3, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_FW_UP_K3, 0);
    // 2. Shift + Up arrow -> \033[5~ (PageUp)
    fake.inject_raw_event(kindle::KeyCatalog::CODE_SHIFT_L, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_FW_UP_K3, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_FW_UP_K3, 0);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_SHIFT_L, 0);
    // 3. Ctrl + C (Aa key on K3 = scancode 190) -> \x03
    fake.inject_raw_event(kindle::KeyCatalog::CODE_AA_CTRL_K3, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_C, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_C, 0);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_AA_CTRL_K3, 0);

    kindle::KeyEvent ev;
    while (mgr.poll(ev)) {
        if (!ev.text.empty() && ev.type == kindle::KeyEventType::Press) {
            pty_stream.append(ev.text);
        }
    }

    // Expected: \033[A (Up) + \033[5~ (PageUp) + \x03 (Ctrl+C)
    std::string expected = "\033[A\033[5~\x03";
    assert(pty_stream == expected);

    fake.close();
    std::cout << "PASS: test_consumer_recipe_kindle_myts\n";
}

void test_debouncer_short_tap_and_bounce_sequence() {
    std::cout << "Testing InputDebouncer short tap & bounce sequence...\n";
    kindle::DebounceConfig config;
    config.debounce_ms = 20;
    kindle::InputDebouncer debouncer(config);

    // 1. Short tap: press at 100ms, release at 110ms (< 20ms)
    kindle::InputEvent press_a{kindle::KeyCode::A, kindle::KeyEventType::Press, 30, 0};
    kindle::InputEvent release_a{kindle::KeyCode::A, kindle::KeyEventType::Release, 30, 0};

    assert(debouncer.filter(press_a, 100));
    // Release at 110ms is inside debounce window -> filter returns false but holds pending
    assert(!debouncer.filter(release_a, 110));
    // At t=115ms (15ms after press), pending release not ready yet
    kindle::InputEvent pending_ev;
    assert(!debouncer.poll_pending(115, pending_ev));
    // At t=120ms (20ms after press), pending release is emitted
    assert(debouncer.poll_pending(120, pending_ev));
    assert(pending_ev.key == kindle::KeyCode::A);
    assert(pending_ev.type == kindle::KeyEventType::Release);

    // 2. Bounce sequence: Press at 200, bounce release at 205, bounce press at 210, real release at 300
    assert(debouncer.filter(press_a, 200));
    assert(!debouncer.filter(release_a, 205)); // suppressed as bounce
    assert(!debouncer.filter(press_a, 210));   // duplicate press suppressed!
    assert(!debouncer.poll_pending(225, pending_ev)); // pending release was cancelled by bounce press!
    assert(debouncer.filter(release_a, 300));  // real release accepted
    std::cout << "PASS: test_debouncer_short_tap_and_bounce_sequence\n";
}

void test_debouncer_per_key_interleaved() {
    std::cout << "Testing InputDebouncer per-key independent timing...\n";
    kindle::DebounceConfig config;
    config.repeat_enabled = true;
    config.repeat_delay_ms = 400;
    config.repeat_interval_ms = 80;
    config.debounce_ms = 20;
    kindle::InputDebouncer debouncer(config);

    kindle::InputEvent press_a{kindle::KeyCode::A, kindle::KeyEventType::Press, 30, 0};
    kindle::InputEvent press_b{kindle::KeyCode::B, kindle::KeyEventType::Press, 48, 0};
    kindle::InputEvent repeat_a{kindle::KeyCode::A, kindle::KeyEventType::Repeat, 30, 0};
    kindle::InputEvent repeat_b{kindle::KeyCode::B, kindle::KeyEventType::Repeat, 48, 0};

    // Press A at t=0, Press B at t=50
    assert(debouncer.filter(press_a, 0));
    assert(debouncer.filter(press_b, 50));

    // At t=400ms: A has reached 400ms delay -> repeat accepted
    assert(debouncer.filter(repeat_a, 400));
    // At t=400ms: B has only reached 350ms (400 - 50) -> repeat rejected!
    assert(!debouncer.filter(repeat_b, 400));

    // At t=450ms: B has reached 400ms delay (450 - 50 = 400) -> repeat accepted!
    assert(debouncer.filter(repeat_b, 450));

    std::cout << "PASS: test_debouncer_per_key_interleaved\n";
}

void test_shortcut_repeat_suppression() {
    std::cout << "Testing KeyboardManager shortcut repeat suppression...\n";
    kindle::FakeInputDevice fake;
    assert(fake.open());

    kindle::DebounceConfig deb_cfg;
    deb_cfg.debounce_ms = 0;
    deb_cfg.repeat_delay_ms = 0;
    kindle::KeyboardManager mgr(fake, kindle::DeviceModel::Kindle3, deb_cfg, kindle::LatchMode::Disabled);

    // 1. Hold Alt, press G -> Ghostbuster on Press
    fake.inject_raw_event(kindle::KeyCatalog::CODE_ALT_L, 1);
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_G, 1);

    kindle::KeyEvent ev;
    assert(mgr.poll(ev)); // Alt press
    assert(mgr.poll(ev)); // G press -> Ghostbuster
    assert(ev.is_shortcut);
    assert(ev.shortcut_action == kindle::ShortcutAction::Ghostbuster);

    // 2. Repeat G while Alt is held -> MUST be suppressed!
    fake.inject_raw_event(kindle::KeyCatalog::CODE_KEY_G, 2);
    assert(!mgr.poll(ev)); // Repeat of Ghostbuster must not be emitted!

    fake.inject_raw_event(kindle::KeyCatalog::CODE_ALT_L, 0);
    assert(mgr.poll(ev)); // Alt release

    // 3. Repeat of Volume keys SHOULD be permitted
    fake.inject_raw_event(kindle::KeyCatalog::CODE_VOL_UP, 1); // Press
    assert(mgr.poll(ev));
    assert(ev.shortcut_action == kindle::ShortcutAction::VolumeUp);

    fake.inject_raw_event(kindle::KeyCatalog::CODE_VOL_UP, 2); // Repeat
    assert(mgr.poll(ev));
    assert(ev.shortcut_action == kindle::ShortcutAction::VolumeUp);

    fake.close();
    std::cout << "PASS: test_shortcut_repeat_suppression\n";
}

void test_modifier_tracker_multi_modifier_chord() {
    std::cout << "Testing ModifierTracker multi-modifier chord latching...\n";
    kindle::ModifierTracker tracker(kindle::LatchMode::StickyOnce);

    // Hold Shift and Alt simultaneously
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Press);
    tracker.update(kindle::KeyCode::Alt, kindle::KeyEventType::Press);
    assert(tracker.state().shift && tracker.state().alt);

    // Type a key while held -> consume latch
    tracker.consume_latch();

    // Release Shift first
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Release);
    assert(!tracker.state().shift);
    assert(tracker.state().alt); // Alt still physically held

    // Release Alt second -> Alt was part of the consumed chord, must NOT latch!
    tracker.update(kindle::KeyCode::Alt, kindle::KeyEventType::Release);
    assert(!tracker.state().alt);
    assert(!tracker.is_latched());

    std::cout << "PASS: test_modifier_tracker_multi_modifier_chord\n";
}

void test_modifier_tracker_lockable_different_modifiers() {
    std::cout << "Testing ModifierTracker Lockable mode with different modifiers...\n";
    kindle::ModifierTracker tracker(kindle::LatchMode::Lockable);

    // Tap Shift
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Press);
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Release);
    assert(tracker.state().shift);
    assert(tracker.is_latched());

    // Press Alt (different modifier) -> must NOT lock!
    tracker.update(kindle::KeyCode::Alt, kindle::KeyEventType::Press);
    tracker.update(kindle::KeyCode::Alt, kindle::KeyEventType::Release);
    // Neither should be locked
    tracker.consume_latch();
    assert(!tracker.state().shift);
    assert(!tracker.state().alt);
    assert(!tracker.is_latched());

    // Double tap Shift -> must lock Shift!
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Press);
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Release);
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Press);
    tracker.update(kindle::KeyCode::Shift, kindle::KeyEventType::Release);

    // Consume latch -> Shift must REMAIN active because it is locked!
    tracker.consume_latch();
    assert(tracker.state().shift);

    std::cout << "PASS: test_modifier_tracker_lockable_different_modifiers\n";
}

int main() {
    test_debouncer_bounce();
    test_debouncer_repeat_disabled();
    test_debouncer_repeat_timing();
    test_modifier_tracker_disabled();
    test_modifier_tracker_sticky_once();
    test_shortcut_registry();
    test_keyboard_manager_pipeline();
    test_consumer_recipe_dino();
    test_consumer_recipe_papergram();
    test_consumer_recipe_kindle_myts();
    test_debouncer_short_tap_and_bounce_sequence();
    test_debouncer_per_key_interleaved();
    test_shortcut_repeat_suppression();
    test_modifier_tracker_multi_modifier_chord();
    test_modifier_tracker_lockable_different_modifiers();
    std::cout << "All InputDebouncer, ModifierTracker, ShortcutRegistry, and KeyboardManager tests passed.\n";
    return 0;
}
