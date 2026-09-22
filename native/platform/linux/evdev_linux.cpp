#include "kindle/input.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <cstring>
#include <vector>

namespace kindle {

// Standard Linux input keycodes mapping to Kindle physical keys
// Kindle Keyboard / DX keycodes from /dev/input/event0
#ifndef KEY_PREVIOUSSONG
#define KEY_PREVIOUSSONG 0x0a5
#endif
#ifndef KEY_NEXTSONG
#define KEY_NEXTSONG 0x0a3
#endif

inline KeyCode map_linux_code_to_kindle(uint16_t code) {
    switch (code) {
        case KEY_UP:
            return KeyCode::Up;
        case KEY_DOWN:
            return KeyCode::Down;
        case KEY_LEFT:
            return KeyCode::Left;
        case KEY_RIGHT:
            return KeyCode::Right;
        case KEY_ENTER:
        case KEY_OK:
            return KeyCode::Select;
        case KEY_PAGEUP:
        case KEY_PREVIOUSSONG: // Often mapped for page back button on K3
            return KeyCode::PageUp;
        case KEY_PAGEDOWN:
        case KEY_NEXTSONG:     // Often mapped for page forward button on K3
            return KeyCode::PageDown;
        case KEY_BACK:
        case KEY_ESC:
            return KeyCode::Back;
        case KEY_MENU:
            return KeyCode::Menu;
        case KEY_HOMEPAGE:
            return KeyCode::Home;
        default:
            return KeyCode::Unknown;
    }
}

inline KeyEventType map_linux_value_to_type(int32_t val) {
    if (val == 1) return KeyEventType::Press;
    if (val == 2) return KeyEventType::Repeat;
    return KeyEventType::Release;
}

class LinuxEvdevInputDevice : public InputDevice {
public:
    LinuxEvdevInputDevice() : fd_(-1) {}

    ~LinuxEvdevInputDevice() override {
        close();
    }

    bool open(const std::string& device_path = "/dev/input/event0") override {
        fd_ = ::open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
        return fd_ >= 0;
    }

    void close() override {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

    bool poll_event(InputEvent& out_event) override {
        if (fd_ < 0) return false;

        struct input_event ev;
        ssize_t bytes = ::read(fd_, &ev, sizeof(ev));
        if (bytes == sizeof(ev)) {
            if (ev.type == EV_KEY) {
                out_event.key = map_linux_code_to_kindle(ev.code);
                out_event.type = map_linux_value_to_type(ev.value);
                return true;
            }
        }
        return false;
    }

private:
    int fd_;
};

class FakeInputDevice : public InputDevice {
public:
    FakeInputDevice() : is_open_(false) {}

    bool open(const std::string& device_path) override {
        is_open_ = true;
        return true;
    }

    void close() override {
        is_open_ = false;
    }

    void inject_raw_event(uint16_t code, int32_t value) {
        InputEvent ev;
        ev.key = map_linux_code_to_kindle(code);
        ev.type = map_linux_value_to_type(value);
        queued_events_.push_back(ev);
    }

    bool poll_event(InputEvent& out_event) override {
        if (!is_open_ || queued_events_.empty()) {
            return false;
        }
        out_event = queued_events_.front();
        queued_events_.erase(queued_events_.begin());
        return true;
    }

private:
    bool is_open_;
    std::vector<InputEvent> queued_events_;
};

} // namespace kindle
