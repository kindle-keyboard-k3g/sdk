#pragma once
#include "kindle/input.hpp"
#include <cstdint>

namespace kindle {

/**
 * Tracks the real-time active state of keyboard modifiers.
 */
struct ModifierState {
    bool shift{false};
    bool ctrl{false};
    bool alt{false};
    bool sym{false};

    static constexpr uint8_t MOD_SHIFT = 1;
    static constexpr uint8_t MOD_CTRL  = 2;
    static constexpr uint8_t MOD_ALT   = 4;
    static constexpr uint8_t MOD_SYM   = 8;

    [[nodiscard]] uint8_t to_mask() const noexcept {
        return (shift ? MOD_SHIFT : 0) | (ctrl ? MOD_CTRL : 0) |
               (alt ? MOD_ALT : 0) | (sym ? MOD_SYM : 0);
    }
};

/**
 * Modifier latching modes for handheld typing ergonomics.
 */
enum class LatchMode : uint8_t {
    Disabled,   ///< Pure physical hold (default for games and terminal)
    StickyOnce, ///< Press & release modifier latches for next keypress only
    Lockable    ///< Double-tap locks modifier (Caps Lock / Alt Lock)
};

/**
 * Internal tracking state for physical and latched modifiers.
 */
struct TrackerState {
    ModifierState current{};
    ModifierState physical{};
    bool          latched{false};
    bool          locked{false};
    bool          consumed_while_held{false};
};

/**
 * Tracks modifier keys (Shift, Alt, Ctrl, Sym) and implements sticky latching
 * for one-handed and handheld typing on Kindle devices.
 */
class ModifierTracker {
public:
    explicit ModifierTracker(LatchMode mode = LatchMode::Disabled) noexcept;

    /**
     * Updates modifier state in response to a key event.
     *
     * @param key Key code of the event.
     * @param type Press or Release action.
     */
    void update(KeyCode key, KeyEventType type) noexcept;

    /**
     * Resets all modifier states and latches.
     */
    void reset() noexcept;

    /**
     * Returns current active modifier state (including latched and locked modifiers).
     */
    [[nodiscard]] const ModifierState& state() const noexcept { return state_.current; }

    /**
     * Checks if any modifier is currently latched for the next keypress.
     */
    [[nodiscard]] bool is_latched() const noexcept { return state_.latched; }

    /**
     * Consumes single-shot latch after a non-modifier key is pressed.
     */
    void consume_latch() noexcept;

private:
    void handle_press(KeyCode key) noexcept;
    void handle_release(KeyCode key) noexcept;
    void set_modifier_flag(ModifierState& s, KeyCode key, bool val) noexcept;

    LatchMode    mode_;
    TrackerState state_{};
};

} // namespace kindle
