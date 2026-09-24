#pragma once
#include "kindle/input.hpp"
#include "kindle/modifier_tracker.hpp"
#include <cstdint>
#include <cstddef>

namespace kindle {

/**
 * Standard Kindle application and global shortcut actions.
 */
enum class ShortcutAction : uint8_t {
    None,
    Ghostbuster,   ///< Alt+G: Full E-Ink refresh
    VolumeUp,      ///< Hardware VolumeUp
    VolumeDown,    ///< Hardware VolumeDown
    Home,          ///< Home / Alt+H
    Back,          ///< Back / Alt+B
    Menu           ///< Menu / Alt+M
};

/**
 * Callback function pointer for custom chord handlers.
 */
using ShortcutHandler = void (*)(void* user_data);

/**
 * Internal binding between modifier mask + KeyCode and an action or callback.
 */
struct ShortcutBinding {
    uint8_t         mod_mask{0};
    KeyCode         key{KeyCode::Unknown};
    ShortcutAction  action{ShortcutAction::None};
    ShortcutHandler handler{nullptr};
    void*           user_data{nullptr};
};

/**
 * Fixed-capacity, zero-allocation registry for keyboard chords and hardware shortcuts.
 */
class ShortcutRegistry {
public:
    ShortcutRegistry() noexcept;

    /**
     * Binds a modifier mask and KeyCode to a predefined ShortcutAction.
     */
    bool bind_action(uint8_t mod_mask, KeyCode key, ShortcutAction action) noexcept;

    /**
     * Binds a modifier mask and KeyCode to a custom callback handler.
     */
    bool bind_callback(uint8_t mod_mask, KeyCode key, ShortcutHandler handler, void* user_data = nullptr) noexcept;

    /**
     * Resets bindings back to factory defaults (Alt+G, Volume keys).
     */
    void reset() noexcept;

    /**
     * Checks if a key and modifier combination matches a registered shortcut chord.
     * Triggers handler if bound, and returns the ShortcutAction.
     */
    ShortcutAction check(KeyCode key, const ModifierState& mods) noexcept;

private:
    void register_defaults() noexcept;

    static constexpr size_t MAX_BINDINGS = 16;
    ShortcutBinding bindings_[MAX_BINDINGS]{};
    size_t          count_{0};
};

} // namespace kindle
