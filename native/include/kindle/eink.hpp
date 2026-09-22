#pragma once
#include <cstdint>
#include <vector>

namespace kindle {

/**
 * Pixel color and depth encodings supported by Kindle E-Ink controllers.
 */
enum class PixelFormat {
    Gray4,   ///< 4-bit per pixel, 16 grayscale levels (standard Pearl/Vizplex mode)
    Gray8,   ///< 8-bit per pixel grayscale
    Gray16   ///< 16-bit packed format
};

/**
 * E-Ink screen refresh waveforms and controller update modes.
 */
enum class RefreshMode {
    Partial, ///< Fast, non-flashing regional update
    Full,    ///< Standard refresh without full black/white inversion
    Fast,    ///< Low-latency UI refresh
    Flash    ///< Complete E-Ink inversion cycle to eliminate ghosting
};

/**
 * Screen rectangular region bounding box.
 */
struct EinkRect {
    uint32_t x;      ///< Horizontal start offset
    uint32_t y;      ///< Vertical start offset
    uint32_t width;  ///< Region width in pixels
    uint32_t height; ///< Region height in pixels
};

/**
 * Abstract interface for interacting with Kindle E-Ink framebuffer and update ioctls.
 */
class EinkDisplay {
public:
    virtual ~EinkDisplay() = default;

    /**
     * Initializes and maps the framebuffer device (/dev/fb0).
     *
     * @return true if opened and mapped successfully, false otherwise.
     */
    virtual bool open() = 0;

    /**
     * Unmaps and closes the framebuffer device.
     */
    virtual void close() = 0;

    /**
     * Triggers an E-Ink display controller update for the specified screen region.
     *
     * @param mode RefreshMode waveform (Partial, Full, Fast, Flash).
     * @param rect Bounding box to update.
     * @return true if ioctl succeeded, false otherwise.
     */
    virtual bool update(RefreshMode mode, const EinkRect& rect) = 0;

    /**
     * Clears the entire E-Ink screen to white.
     */
    virtual void clear_screen() = 0;

    /**
     * Returns raw pointer to memory-mapped framebuffer.
     *
     * @return Pointer to framebuffer bytes.
     */
    virtual uint8_t* get_framebuffer() = 0;

    /**
     * Returns display horizontal resolution in pixels.
     *
     * @return Width in pixels.
     */
    virtual uint32_t get_width() const = 0;

    /**
     * Returns display vertical resolution in pixels.
     *
     * @return Height in pixels.
     */
    virtual uint32_t get_height() const = 0;

    /**
     * Returns active pixel format of the display.
     *
     * @return PixelFormat enum.
     */
    virtual PixelFormat get_format() const = 0;
};

} // namespace kindle
