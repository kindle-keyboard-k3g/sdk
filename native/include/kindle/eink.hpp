#pragma once
#include <cstdint>
#include <vector>

namespace kindle {

enum class PixelFormat {
    Gray4,   // 4bpp, 16 grayscale levels
    Gray8,   // 8bpp
    Gray16   // 16bpp
};

enum class RefreshMode {
    Partial,
    Full,
    Fast,
    Flash
};

struct EinkRect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

class EinkDisplay {
public:
    virtual ~EinkDisplay() = default;
    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool update(RefreshMode mode, const EinkRect& rect) = 0;
    virtual void clear_screen() = 0;
    virtual uint8_t* get_framebuffer() = 0;
    virtual uint32_t get_width() const = 0;
    virtual uint32_t get_height() const = 0;
    virtual PixelFormat get_format() const = 0;
};

} // namespace kindle
