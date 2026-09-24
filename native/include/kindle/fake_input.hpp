#pragma once
#include "kindle/input.hpp"
#include "kindle/key_catalog.hpp"
#include <vector>
#include <string>

namespace kindle {

/**
 * Deterministic in-memory fake input device for unit tests and host platforms.
 * Simulates multi-device event injection, scancode mapping, and EVIOCGRAB states.
 */
class FakeInputDevice : public InputDevice {
public:
    explicit FakeInputDevice(DeviceModel model = DeviceModel::Kindle3);

    bool open(const std::string& path = "/dev/fake") override;
    void close() override;

    bool grab();
    bool release();
    [[nodiscard]] bool is_grabbed() const noexcept { return grabbed_; }

    /**
     * Injects a raw Linux evdev scancode and value (1=press, 0=release, 2=repeat).
     */
    void inject_raw_event(uint16_t code, int32_t value);

    /**
     * Injects a high-level KeyCode and KeyEventType directly.
     */
    void inject_key(KeyCode key, KeyEventType type = KeyEventType::Press);

    bool poll_event(InputEvent& out_event) override;

private:
    DeviceModel             model_;
    bool                    is_open_{false};
    bool                    grabbed_{false};
    std::vector<InputEvent> queue_;
};

} // namespace kindle
