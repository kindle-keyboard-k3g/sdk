#include "kindle/linux_input.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <algorithm>

namespace kindle {

LinuxEvdevInputDevice::LinuxEvdevInputDevice(
    std::vector<std::string> device_paths,
    bool grab_exclusive,
    DeviceModel model)
    : device_paths_(std::move(device_paths)),
      grab_exclusive_(grab_exclusive),
      model_(model) {
    buffered_events_.reserve(16);
}

LinuxEvdevInputDevice::~LinuxEvdevInputDevice() {
    close();
}

bool LinuxEvdevInputDevice::open(const std::string& single_path) {
    device_paths_ = {single_path};
    return open_devices();
}

bool LinuxEvdevInputDevice::open_devices() {
    close();
    for (const auto& path : device_paths_) {
        int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            fds_.push_back(fd);
        }
    }

    if (fds_.empty()) {
        return false;
    }

    if (grab_exclusive_) {
        grab();
    }

    return true;
}

void LinuxEvdevInputDevice::close() {
    if (grabbed_) {
        release();
    }

    for (int fd : fds_) {
        if (fd >= 0) {
            ::close(fd);
        }
    }
    fds_.clear();
    buffered_events_.clear();
}

bool LinuxEvdevInputDevice::grab() {
    bool all_ok = true;
    for (int fd : fds_) {
        if (fd >= 0) {
            if (::ioctl(fd, EVIOCGRAB, 1) < 0) {
                all_ok = false;
            }
        }
    }
    grabbed_ = true;
    return all_ok;
}

bool LinuxEvdevInputDevice::release() {
    bool all_ok = true;
    for (int fd : fds_) {
        if (fd >= 0) {
            if (::ioctl(fd, EVIOCGRAB, 0) < 0) {
                all_ok = false;
            }
        }
    }
    grabbed_ = false;
    return all_ok;
}

bool LinuxEvdevInputDevice::poll_event(InputEvent& out_event) {
    return poll_event(out_event, 0);
}

bool LinuxEvdevInputDevice::poll_event(InputEvent& out_event, int timeout_ms) {
    if (!buffered_events_.empty()) {
        out_event = buffered_events_.front();
        buffered_events_.erase(buffered_events_.begin());
        return true;
    }

    if (fds_.empty()) {
        return false;
    }

    constexpr size_t MAX_FDS = 16;
    struct pollfd pfd[MAX_FDS];
    nfds_t nfds = static_cast<nfds_t>(std::min(fds_.size(), MAX_FDS));

    for (nfds_t i = 0; i < nfds; ++i) {
        pfd[i].fd = fds_[i];
        pfd[i].events = POLLIN;
        pfd[i].revents = 0;
    }

    int ret = ::poll(pfd, nfds, timeout_ms);
    if (ret <= 0) {
        return false;
    }

    struct input_event buf[8];
    for (nfds_t i = 0; i < nfds; ++i) {
        if (pfd[i].revents & POLLIN) {
            ssize_t bytes = ::read(pfd[i].fd, buf, sizeof(buf));
            if (bytes > 0) {
                size_t n_events = static_cast<size_t>(bytes) / sizeof(struct input_event);
                for (size_t j = 0; j < n_events; ++j) {
                    if (buf[j].type == EV_KEY) {
                        KeyEventType type = KeyEventType::Release;
                        if (buf[j].value == 1) {
                            type = KeyEventType::Press;
                        } else if (buf[j].value == 2) {
                            type = KeyEventType::Repeat;
                        }

                        KeyCode key = KeyCatalog::map_scancode(buf[j].code, model_);
                        buffered_events_.push_back(InputEvent{key, type, buf[j].code});
                    }
                }
            }
        }
    }

    if (!buffered_events_.empty()) {
        out_event = buffered_events_.front();
        buffered_events_.erase(buffered_events_.begin());
        return true;
    }

    return false;
}

} // namespace kindle
