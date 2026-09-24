#pragma once
#include "kindle/input.hpp"
#include "kindle/input_debouncer.hpp"
#include "kindle/modifier_tracker.hpp"
#include "kindle/keyboard_translator.hpp"
#include "kindle/shortcut_registry.hpp"
#include <string_view>

namespace kindle {

/**
 * High-level rich keyboard event processed through debounce, modifier tracking,
 * shortcut dispatch, and character/ANSI translation.
 */
struct KeyEvent {
    KeyCode          key{KeyCode::Unknown};
    KeyEventType     type{KeyEventType::Press};
    ModifierState    modifiers{};
    std::string_view text{};
    ShortcutAction   shortcut_action{ShortcutAction::None};
    bool             is_shortcut{false};
};

/**
 * Unified Keyboard Management Facade. Coordinates evdev input, debouncing,
 * modifier tracking (with sticky latch support), shortcut chords, and zero-allocation
 * character/escape translation.
 */
class KeyboardManager {
public:
    explicit KeyboardManager(
        InputDevice& device,
        DeviceModel model = DeviceModel::Kindle3,
        const DebounceConfig& debounce = {},
        LatchMode latch = LatchMode::Disabled) noexcept;

    /**
     * Polls the next high-level KeyEvent.
     * Returns false if no event is available.
     * Zero heap allocation on hot path.
     */
    bool poll(KeyEvent& out_event) noexcept;

    /**
     * Accessor to the shortcut registry for custom application chord bindings.
     */
    ShortcutRegistry& shortcuts() noexcept { return shortcuts_; }

    /**
     * Accessor to current modifier state.
     */
    [[nodiscard]] const ModifierState& modifiers() const noexcept { return tracker_.state(); }

    /**
     * Resets debouncer, modifier tracker, and shortcuts to default state.
     */
    void reset() noexcept;

private:
    bool process_raw_event(const InputEvent& raw_ev, KeyEvent& out_event) noexcept;
    bool process_modifier(const InputEvent& raw_ev, KeyEvent& out_event) noexcept;
    bool process_key(const InputEvent& raw_ev, KeyEvent& out_event) noexcept;
    static uint64_t current_time_ms(const InputEvent& raw_ev) noexcept;

    InputDevice&       device_;
    InputDebouncer     debouncer_;
    ModifierTracker    tracker_;
    KeyboardTranslator translator_;
    ShortcutRegistry   shortcuts_;
};

} // namespace kindle
