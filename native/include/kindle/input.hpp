#pragma once
#include <cstdint>
#include <string>

namespace kindle {

/**
 * Hardware key codes for Kindle keypad and button events.
 */
enum class KeyCode : uint16_t {
    // Alphanumeric keys
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Punctuation & Editing
    Enter,
    Space,
    Backspace,
    Dot,
    Slash,

    // Modifiers
    Shift,
    Alt,
    Sym,
    Ctrl,

    // Navigation & Kindle Buttons
    Up,
    Down,
    Left,
    Right,
    Select,
    PageUp,
    PageDown,
    Back,
    Menu,
    Home,

    // Hardware functions
    VolumeUp,
    VolumeDown,
    Power,

    Unknown
};

/**
 * Key press, release, and auto-repeat event classifications.
 */
enum class KeyEventType : uint8_t {
    Press,   ///< Key pressed down
    Release, ///< Key released
    Repeat   ///< Key held down and repeating
};

/**
 * Decoded keyboard or hardware button event.
 */
struct InputEvent {
    KeyCode key{KeyCode::Unknown};       ///< Hardware key code
    KeyEventType type{KeyEventType::Press}; ///< Event action type (press, release, repeat)
    uint16_t raw_code{0};                ///< Raw Linux evdev scancode
};

/**
 * Abstract interface for reading raw input events from Linux evdev nodes (/dev/input/event*).
 */
class InputDevice {
public:
    virtual ~InputDevice() = default;

    /**
     * Opens the specified input event device file.
     *
     * @param device_path Path to evdev device node (e.g. "/dev/input/event0").
     * @return true if opened successfully, false otherwise.
     */
    virtual bool open(const std::string& device_path) = 0;

    /**
     * Closes the active input event device file descriptor.
     */
    virtual void close() = 0;

    /**
     * Non-blocking or blocking poll for the next decoded input event.
     *
     * @param out_event Reference populated with decoded key code and action.
     * @return true if an event was read and decoded, false on EOF or would-block.
     */
    virtual bool poll_event(InputEvent& out_event) = 0;
};

} // namespace kindle
