// Implementation of LinuxEinkDisplay.
// Maps /dev/fb0 and issues Kindle EPDC ioctls.
#include "kindle/linux_eink.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <cstring>
#include <utility>

namespace kindle {

// Legacy Kindle E-Ink IOCTL definitions
#define FBIO_EINK_UPDATE_DISPLAY      0x46DB   // full-screen flash
#define FBIO_EINK_UPDATE_DISPLAY_AREA 0x46DD   // partial/area update
#define FBIO_EINK_CLEAR_SCREEN        0x46E1

struct eink_update_area {
    int x1, y1, x2, y2, mode;
};

LinuxEinkDisplay::LinuxEinkDisplay(std::string device_path, bool fallback)
    : device_path_(std::move(device_path)), allow_fallback_(fallback) {}

LinuxEinkDisplay::~LinuxEinkDisplay() {
    close();
}

bool LinuxEinkDisplay::open() {
    fb_fd_ = ::open(device_path_.c_str(), O_RDWR);
    if (fb_fd_ < 0) {
        if (!allow_fallback_) return false;
        // Fallback: allocate in-memory buffer
        fallback_active_ = true;
        screen_size_ = width_ * height_ / 2;
        fallback_buf_.assign(screen_size_, 0xFF);
        fb_ptr_ = fallback_buf_.data();
        return true;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fb_fd_, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        ::close(fb_fd_);
        fb_fd_ = -1;
        if (!allow_fallback_) return false;
        fallback_active_ = true;
        screen_size_ = width_ * height_ / 2;
        fallback_buf_.assign(screen_size_, 0xFF);
        fb_ptr_ = fallback_buf_.data();
        return true;
    }

    width_  = vinfo.xres;
    height_ = vinfo.yres;
    screen_size_ = width_ * height_ / 2;  // 4bpp

    fb_ptr_ = static_cast<uint8_t*>(
        mmap(nullptr, screen_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd_, 0));
    if (fb_ptr_ == MAP_FAILED) {
        fb_ptr_ = nullptr;
        ::close(fb_fd_);
        fb_fd_ = -1;
        if (!allow_fallback_) return false;
        fallback_active_ = true;
        fallback_buf_.assign(screen_size_, 0xFF);
        fb_ptr_ = fallback_buf_.data();
    }
    return true;
}

void LinuxEinkDisplay::close() {
    if (!fallback_active_ && fb_ptr_ && fb_ptr_ != MAP_FAILED) {
        munmap(fb_ptr_, screen_size_);
        fb_ptr_ = nullptr;
    }
    if (fb_fd_ >= 0) {
        ::close(fb_fd_);
        fb_fd_ = -1;
    }
    fallback_active_ = false;
    fallback_buf_.clear();
}

bool LinuxEinkDisplay::update(RefreshMode mode, const EinkRect& rect) {
    if (fallback_active_) return true;
    if (fb_fd_ < 0) return false;

    if (mode == RefreshMode::Flash) {
        // Full-screen inversion cycle to eliminate ghosting
        return ioctl(fb_fd_, FBIO_EINK_UPDATE_DISPLAY, 0) == 0;
    }

    eink_update_area area;
    area.x1   = static_cast<int>(rect.x);
    area.y1   = static_cast<int>(rect.y);
    area.x2   = static_cast<int>(rect.x + rect.width);
    area.y2   = static_cast<int>(rect.y + rect.height);
    area.mode = (mode == RefreshMode::Full) ? 1 : 0;  // Full→GC16, Partial/Fast→DU

    return ioctl(fb_fd_, FBIO_EINK_UPDATE_DISPLAY_AREA, &area) == 0;
}

void LinuxEinkDisplay::clear_screen() {
    if (fallback_active_) {
        std::fill(fallback_buf_.begin(), fallback_buf_.end(), 0xFF);
        return;
    }
    if (fb_fd_ >= 0) ioctl(fb_fd_, FBIO_EINK_CLEAR_SCREEN, 0);
}

uint8_t* LinuxEinkDisplay::get_framebuffer() { return fb_ptr_; }
uint32_t LinuxEinkDisplay::get_width()  const { return width_; }
uint32_t LinuxEinkDisplay::get_height() const { return height_; }
PixelFormat LinuxEinkDisplay::get_format() const { return PixelFormat::Gray4; }

} // namespace kindle
