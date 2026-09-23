// native/include/kindle/fake_eink.hpp
#pragma once
#include "kindle/eink.hpp"
#include <cstdint>
#include <cstring>
#include <vector>

namespace kindle {

class FakeEinkDisplay : public EinkDisplay {
public:
    FakeEinkDisplay(uint32_t width, uint32_t height,
                    PixelFormat format = PixelFormat::Gray4);

    bool open() override;
    void close() override;
    bool update(RefreshMode mode, const EinkRect& rect) override;
    void clear_screen() override;
    uint8_t* get_framebuffer() override;
    uint32_t get_width()  const override;
    uint32_t get_height() const override;
    PixelFormat get_format() const override;

    // Test inspection
    uint32_t    get_update_count() const { return update_count_; }
    RefreshMode get_last_mode()    const { return last_mode_; }
    EinkRect    get_last_rect()    const { return last_rect_; }

private:
    uint32_t             width_, height_;
    PixelFormat          format_;
    bool                 is_open_ = false;
    std::vector<uint8_t> buffer_;
    uint32_t             update_count_ = 0;
    RefreshMode          last_mode_    = RefreshMode::Partial;
    EinkRect             last_rect_    = {0, 0, 0, 0};
};

inline FakeEinkDisplay::FakeEinkDisplay(uint32_t width, uint32_t height,
                                         PixelFormat format)
    : width_(width), height_(height), format_(format) {
    size_t bytes = (format == PixelFormat::Gray4) ? (width * height / 2)
                                                  : (width * height);
    buffer_.resize(bytes, 0xFF);
}

inline bool FakeEinkDisplay::open()  { is_open_ = true;  return true; }
inline void FakeEinkDisplay::close() { is_open_ = false; }

inline bool FakeEinkDisplay::update(RefreshMode mode, const EinkRect& rect) {
    if (!is_open_) return false;
    last_mode_ = mode;
    last_rect_ = rect;
    ++update_count_;
    return true;
}

inline void FakeEinkDisplay::clear_screen() {
    std::memset(buffer_.data(), 0xFF, buffer_.size());
    update(RefreshMode::Full, {0, 0, width_, height_});
}

inline uint8_t* FakeEinkDisplay::get_framebuffer() { return buffer_.data(); }
inline uint32_t FakeEinkDisplay::get_width()  const { return width_; }
inline uint32_t FakeEinkDisplay::get_height() const { return height_; }
inline PixelFormat FakeEinkDisplay::get_format() const { return format_; }

} // namespace kindle
