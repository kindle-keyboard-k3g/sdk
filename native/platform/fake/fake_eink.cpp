#include "kindle/eink.hpp"
#include <vector>
#include <cstring>

namespace kindle {

class FakeEinkDisplay : public EinkDisplay {
public:
    FakeEinkDisplay(uint32_t width, uint32_t height, PixelFormat format)
        : width_(width), height_(height), format_(format), is_open_(false) {
        size_t bytes = (format == PixelFormat::Gray4) ? (width * height / 2) : (width * height);
        buffer_.resize(bytes, 0xFF); // Initialize white screen
    }

    bool open() override {
        is_open_ = true;
        return true;
    }

    void close() override {
        is_open_ = false;
    }

    bool update(RefreshMode mode, const EinkRect& rect) override {
        if (!is_open_) return false;
        last_mode_ = mode;
        last_rect_ = rect;
        update_count_++;
        return true;
    }

    void clear_screen() override {
        std::memset(buffer_.data(), 0xFF, buffer_.size());
        update(RefreshMode::Full, {0, 0, width_, height_});
    }

    uint8_t* get_framebuffer() override {
        return buffer_.data();
    }

    uint32_t get_width() const override { return width_; }
    uint32_t get_height() const override { return height_; }
    PixelFormat get_format() const override { return format_; }

    uint32_t get_update_count() const { return update_count_; }
    RefreshMode get_last_mode() const { return last_mode_; }

private:
    uint32_t width_;
    uint32_t height_;
    PixelFormat format_;
    bool is_open_;
    std::vector<uint8_t> buffer_;
    uint32_t update_count_ = 0;
    RefreshMode last_mode_ = RefreshMode::Partial;
    EinkRect last_rect_ = {0, 0, 0, 0};
};

} // namespace kindle
