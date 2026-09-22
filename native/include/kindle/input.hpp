#pragma once
#include <cstdint>
#include <string>

namespace kindle {

enum class KeyCode {
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
    Unknown
};

enum class KeyEventType {
    Press,
    Release,
    Repeat
};

struct InputEvent {
    KeyCode key;
    KeyEventType type;
};

class InputDevice {
public:
    virtual ~InputDevice() = default;
    virtual bool open(const std::string& device_path) = 0;
    virtual void close() = 0;
    virtual bool poll_event(InputEvent& out_event) = 0;
};

} // namespace kindle
