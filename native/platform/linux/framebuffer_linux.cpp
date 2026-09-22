#include "kindle/eink.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <cstring>

namespace kindle {

// Legacy Kindle E-Ink IOCTL definitions
#define FBIO_EINK_UPDATE_DISPLAY_LEGACY      0x46db
#define FBIO_EINK_UPDATE_DISPLAY_AREA_LEGACY 0x46dd
#define FBIO_EINK_CLEAR_SCREEN_LEGACY        0x46e1

struct eink_update_area {
    int x1;
    int y1;
    int x2;
    int y2;
    int mode;
};

class LinuxEinkDisplay : public EinkDisplay {
public:
    LinuxEinkDisplay(uint32_t width = 600, uint32_t height = 800)
        : width_(width), height_(height), fb_fd_(-1), fb_ptr_(nullptr), screen_size_(0) {}

    ~LinuxEinkDisplay() override {
        close();
    }

    bool open() override {
        fb_fd_ = ::open("/dev/fb0", O_RDWR);
        if (fb_fd_ < 0) return false;

        struct fb_var_screeninfo vinfo;
        if (ioctl(fb_fd_, FBIOGET_VSCREENINFO, &vinfo) < 0) {
            ::close(fb_fd_);
            fb_fd_ = -1;
            return false;
        }

        width_ = vinfo.xres;
        height_ = vinfo.yres;
        screen_size_ = width_ * height_ / 2; // Assuming 4bpp

        fb_ptr_ = static_cast<uint8_t*>(mmap(nullptr, screen_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd_, 0));
        return fb_ptr_ != MAP_FAILED;
    }

    void close() override {
        if (fb_ptr_ && fb_ptr_ != MAP_FAILED) {
            munmap(fb_ptr_, screen_size_);
            fb_ptr_ = nullptr;
        }
        if (fb_fd_ >= 0) {
            ::close(fb_fd_);
            fb_fd_ = -1;
        }
    }

    bool update(RefreshMode mode, const EinkRect& rect) override {
        if (fb_fd_ < 0) return false;

        struct eink_update_area area;
        area.x1 = rect.x;
        area.y1 = rect.y;
        area.x2 = rect.x + rect.width;
        area.y2 = rect.y + rect.height;
        area.mode = (mode == RefreshMode::Full) ? 1 : 0;

        return ioctl(fb_fd_, FBIO_EINK_UPDATE_DISPLAY_AREA_LEGACY, &area) == 0;
    }

    void clear_screen() override {
        if (fb_fd_ >= 0) {
            ioctl(fb_fd_, FBIO_EINK_CLEAR_SCREEN_LEGACY, 0);
        }
    }

    uint8_t* get_framebuffer() override {
        return fb_ptr_;
    }

    uint32_t get_width() const override { return width_; }
    uint32_t get_height() const override { return height_; }
    PixelFormat get_format() const override { return PixelFormat::Gray4; }

private:
    uint32_t width_;
    uint32_t height_;
    int fb_fd_;
    uint8_t* fb_ptr_;
    size_t screen_size_;
};

} // namespace kindle
