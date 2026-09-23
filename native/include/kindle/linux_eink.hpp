// native/include/kindle/linux_eink.hpp
#pragma once
#include "kindle/eink.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace kindle {

class LinuxEinkDisplay : public EinkDisplay {
public:
    explicit LinuxEinkDisplay(std::string device_path = "/dev/fb0",
                               bool fallback = false);
    ~LinuxEinkDisplay() override;

    bool open() override;
    void close() override;
    bool update(RefreshMode mode, const EinkRect& rect) override;
    void clear_screen() override;
    uint8_t* get_framebuffer() override;
    uint32_t get_width()  const override;
    uint32_t get_height() const override;
    PixelFormat get_format() const override;

    bool is_using_fallback() const { return fallback_active_; }

private:
    std::string           device_path_;
    bool                  allow_fallback_;
    bool                  fallback_active_ = false;
    int                   fb_fd_           = -1;
    uint8_t*              fb_ptr_          = nullptr;
    std::vector<uint8_t>  fallback_buf_;
    uint32_t              width_           = 600;
    uint32_t              height_          = 800;
    size_t                screen_size_     = 0;
};

} // namespace kindle
