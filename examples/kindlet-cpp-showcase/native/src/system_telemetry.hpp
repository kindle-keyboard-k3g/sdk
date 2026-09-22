#pragma once

#include "kindle/hardware.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace showcase {

/**
 * Grayscale test pattern parameters and generated pixel buffer.
 */
struct EinkPattern {
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> pixels; // 8-bit grayscale values 0-255 (mapped to 16 levels: 0, 17, 34, ... 255)
};

/**
 * System hardware telemetry collector and E-Ink pattern generator.
 */
class SystemTelemetry {
public:
    /**
     * Collects current SoC, CPU architecture, and RAM metrics.
     * Returns JSON-formatted telemetry string.
     */
    static std::string query_telemetry_json();

    /**
     * Generates a 16-level grayscale test pattern (default: 200x120 pixels).
     * Creates horizontal gradients, stepped gray bars (16 levels), and geometric alignment marks.
     *
     * @param width image width in pixels
     * @param height image height in pixels
     * @param pattern_type pattern index (0 = stepped bars, 1 = radial gradient, 2 = checkerboard)
     * @return EinkPattern structure
     */
    static EinkPattern generate_pattern(uint32_t width = 200, uint32_t height = 120, int pattern_type = 0);

    /**
     * Encodes EinkPattern pixels into a comma-separated or hex string for simple IPC transmission.
     */
    static std::string encode_pattern_hex(const EinkPattern& pattern);
};

} // namespace showcase
