#include "kindle/fake_input.hpp"

namespace kindle {

FakeInputDevice::FakeInputDevice(DeviceModel model)
    : model_(model) {}

bool FakeInputDevice::open(const std::string& path) {
    (void)path;
    is_open_ = true;
    return true;
}

void FakeInputDevice::close() {
    is_open_ = false;
    grabbed_ = false;
    queue_.clear();
}

bool FakeInputDevice::grab() {
    grabbed_ = true;
    return true;
}

bool FakeInputDevice::release() {
    grabbed_ = false;
    return true;
}

void FakeInputDevice::inject_raw_event(uint16_t code, int32_t value) {
    KeyEventType type = KeyEventType::Release;
    if (value == 1) {
        type = KeyEventType::Press;
    } else if (value == 2) {
        type = KeyEventType::Repeat;
    }

    KeyCode key = KeyCatalog::map_scancode(code, model_);
    queue_.push_back(InputEvent{key, type, code});
}

void FakeInputDevice::inject_key(KeyCode key, KeyEventType type) {
    queue_.push_back(InputEvent{key, type, 0});
}

bool FakeInputDevice::poll_event(InputEvent& out_event) {
    if (!is_open_ || queue_.empty()) {
        return false;
    }
    out_event = queue_.front();
    queue_.erase(queue_.begin());
    return true;
}

} // namespace kindle
