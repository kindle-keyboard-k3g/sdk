#include "kindle/keyboard_manager.hpp"
#include <chrono>

namespace kindle {

static constexpr bool is_modifier(KeyCode key) noexcept {
    return key == KeyCode::Shift || key == KeyCode::Alt ||
           key == KeyCode::Ctrl  || key == KeyCode::Sym;
}

KeyboardManager::KeyboardManager(
    InputDevice& device,
    DeviceModel model,
    const DebounceConfig& debounce,
    LatchMode latch) noexcept
    : device_(device),
      debouncer_(debounce),
      tracker_(latch),
      translator_(model),
      shortcuts_() {}

void KeyboardManager::reset() noexcept {
    debouncer_.reset();
    tracker_.reset();
    translator_.reset_modifiers();
    shortcuts_.reset();
}

uint64_t KeyboardManager::current_time_ms(const InputEvent& raw_ev) noexcept {
    if (raw_ev.timestamp_us > 0) {
        return raw_ev.timestamp_us / 1000ULL;
    }
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

bool KeyboardManager::process_modifier(const InputEvent& raw_ev, KeyEvent& out_event) noexcept {
    tracker_.update(raw_ev.key, raw_ev.type);
    out_event.modifiers = tracker_.state();
    return true;
}

bool KeyboardManager::process_key(const InputEvent& raw_ev, KeyEvent& out_event) noexcept {
    out_event.modifiers = tracker_.state();
    if (raw_ev.type == KeyEventType::Release) return true;
    if (shortcuts_.has_binding(raw_ev.key, tracker_.state())) {
        ShortcutAction act = shortcuts_.check(raw_ev.key, tracker_.state());
        if (raw_ev.type == KeyEventType::Repeat) {
            if (act != ShortcutAction::VolumeUp && act != ShortcutAction::VolumeDown) {
                return false;
            }
        }
        out_event.shortcut_action = act;
        out_event.is_shortcut = true;
        tracker_.consume_latch();
        return true;
    }
    out_event.text = translator_.translate(raw_ev.key, tracker_.state());
    tracker_.consume_latch();
    return true;
}

bool KeyboardManager::process_raw_event(const InputEvent& raw_ev, KeyEvent& out_event) noexcept {
    out_event = KeyEvent{raw_ev.key, raw_ev.type, {}, {}, ShortcutAction::None, false};
    if (is_modifier(raw_ev.key)) {
        return process_modifier(raw_ev, out_event);
    }
    return process_key(raw_ev, out_event);
}

bool KeyboardManager::poll(KeyEvent& out_event) noexcept {
    uint64_t now_ms = current_time_ms(InputEvent{});
    InputEvent pending_ev;
    if (debouncer_.poll_pending(now_ms, pending_ev)) {
        return process_raw_event(pending_ev, out_event);
    }
    InputEvent raw_ev;
    while (device_.poll_event(raw_ev)) {
        uint64_t time_ms = current_time_ms(raw_ev);
        if (!debouncer_.filter(raw_ev, time_ms)) {
            continue;
        }
        if (!process_raw_event(raw_ev, out_event)) {
            continue;
        }
        return true;
    }
    return false;
}

} // namespace kindle
