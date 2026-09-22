#pragma once
#include <cstdint>
#include <string>

namespace kindle {

/**
 * Hardware key codes for Kindle keypad and button events.
 */
enum class KeyCode {
    Up,        ///< 5-way D-Pad Up
    Down,      ///< 5-way D-Pad Down
    Left,      ///< 5-way D-Pad Left
    Right,     ///< 5-way D-Pad Right
    Select,    ///< 5-way D-Pad Center Select
    PageUp,    ///< Previous page button
    PageDown,  ///< Next page button
    Back,      ///< Dedicated Back button
    Menu,      ///< Menu button
    Home,      ///< Home button
    Unknown    ///< Unrecognized or unsupported scan code
};

/**
 * Key press, release, and auto-repeat event classifications.
 */
enum class KeyEventType {
    Press,   ///< Key pressed down
    Release, ///< Key released
    Repeat   ///< Key held down and repeating
};

/**
 * Decoded keyboard or hardware button event.
 */
struct InputEvent {
    KeyCode key;        ///< Hardware key code
    KeyEventType type;  ///< Event action type (press, release, repeat)
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
