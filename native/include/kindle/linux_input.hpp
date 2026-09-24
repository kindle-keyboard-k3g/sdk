#pragma once
#include "kindle/input.hpp"
#include "kindle/key_catalog.hpp"
#include <array>
#include <vector>
#include <string>

namespace kindle {

/**
 * Multi-node Linux evdev input driver for Kindle devices.
 * Multiplexes event0 (Keyboard), event1 (5-way D-Pad), and event2 (Page Turn Rockers)
 * using poll(), batch reads struct input_event, and manages exclusive hardware grabbing (EVIOCGRAB).
 */
class LinuxEvdevInputDevice : public InputDevice {
public:
    explicit LinuxEvdevInputDevice(
        std::vector<std::string> device_paths = {
            "/dev/input/event0", "/dev/input/event1", "/dev/input/event2"
        },
        bool grab_exclusive = true,
        DeviceModel model = DeviceModel::Kindle3);

    ~LinuxEvdevInputDevice() override;

    /**
     * Opens a single device node path (implements InputDevice).
     */
    bool open(const std::string& single_path) override;

    /**
     * Opens all configured device node paths for multiplexed polling.
     */
    bool open_devices();

    /**
     * Releases any active EVIOCGRAB locks and closes all open file descriptors.
     */
    void close() override;

    /**
     * Grabs exclusive access (ioctl EVIOCGRAB) across all open descriptors.
     */
    bool grab();

    /**
     * Releases exclusive access (ioctl EVIOCGRAB 0) across all open descriptors.
     */
    bool release();

    [[nodiscard]] bool is_grabbed() const noexcept { return grabbed_; }

    /**
     * Polls with non-blocking timeout (0 ms).
     */
    bool poll_event(InputEvent& out_event) override;

    /**
     * Polls for input events across all descriptors with a timeout in milliseconds.
     * timeout_ms = -1 blocks indefinitely; 0 returns immediately.
     */
    bool poll_event(InputEvent& out_event, int timeout_ms);

private:
    static constexpr size_t BUFFER_CAPACITY = 256;

    std::vector<std::string>                device_paths_;
    std::vector<int>                        fds_;
    bool                                    grab_exclusive_;
    bool                                    grabbed_{false};
    DeviceModel                             model_;
    std::array<InputEvent, BUFFER_CAPACITY> buffered_events_{};
    size_t                                  buffer_head_{0};
    size_t                                  buffer_count_{0};
};

} // namespace kindle
